/*
 * Copyright (C) 2001,2002 Nikos Mavroyanopoulos <nmav@hellug.gr>
 *
 * This file is part of GNUTLS.
 *
 *  The GNUTLS library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public   
 *  License as published by the Free Software Foundation; either 
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 */

/* All functions which relate to X.509 certificate verification stuff are
 * included here
 */

#include "gnutls_int.h"
#include "gnutls_errors.h"
#include "gnutls_cert.h"
#include "libtasn1.h"
#include "gnutls_global.h"
#include "gnutls_num.h"		/* GMAX */
#include <gnutls_sig.h>
#include <gnutls_str.h>


/* Return 0 or INVALID, if the issuer is a CA,
 * or not.
 */
static int check_if_ca(const gnutls_cert * cert,
		       const gnutls_cert * issuer)
{
	CertificateStatus ret = GNUTLS_CERT_INVALID;

	/* Check if the issuer is the same with the
	 * certificate. This is added in order for trusted
	 * certificates to be able to verify themselves.
	 */
	if (cert->raw.size == issuer->raw.size) {
		if (memcmp
		    (cert->raw.data, issuer->raw.data,
		     cert->raw.size) == 0) {
			return 0;
		}
	}

	if (issuer->CA == 1) {
		ret = 0;
	} else
		gnutls_assert();

	return ret;
}



void _gnutls_int2str(int k, char *data);

#define MAX_DN_ELEM 1024

/* This function checks if 'certs' issuer is 'issuer_cert'.
 * This does a straight (DER) compare of the issuer/subject fields in
 * the given certificates.
 *
 * FIXME: use a real DN comparison algorithm.
 */
static
int compare_dn(gnutls_cert * cert, gnutls_cert * issuer_cert)
{
	ASN1_TYPE c2, c3;
	int result, len1;
	int len2;
	char tmpstr[512];
	int start1, start2, end1, end2;

	/* get the issuer of 'cert'
	 */
	if ((result =
	     _gnutls_asn1_create_element(_gnutls_get_pkix(), "PKIX1.Certificate",
				   &c2, "certificate2")) != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result = asn1_der_decoding(&c2, cert->raw.data, cert->raw.size, NULL);
	if (result != ASN1_SUCCESS) {
		/* couldn't decode DER */
		gnutls_assert();
		asn1_delete_structure(&c2);
		return _gnutls_asn2err(result);
	}



	/* get the 'subject' info of 'issuer_cert'
	 */
	if ((result =
	     _gnutls_asn1_create_element(_gnutls_get_pkix(), "PKIX1.Certificate",
				   &c3, "certificate2")) != ASN1_SUCCESS) {
		gnutls_assert();
		asn1_delete_structure(&c2);
		return _gnutls_asn2err(result);
	}

	result =
	    asn1_der_decoding(&c3, issuer_cert->raw.data, issuer_cert->raw.size, NULL);
	if (result != ASN1_SUCCESS) {
		/* couldn't decode DER */
		gnutls_assert();
		asn1_delete_structure(&c2);
		return _gnutls_asn2err(result);
	}


	_gnutls_str_cpy(tmpstr, sizeof(tmpstr),
			"certificate2.tbsCertificate.issuer");
	result =
	    asn1_der_decoding_startEnd(c2, cert->raw.data, cert->raw.size,
				   tmpstr, &start1, &end1);
	asn1_delete_structure(&c2);

	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		asn1_delete_structure(&c3);
		return _gnutls_asn2err(result);
	}

	len1 = end1 - start1 + 1;

	_gnutls_str_cpy(tmpstr, sizeof(tmpstr),
			"certificate2.tbsCertificate.subject");
	result =
	    asn1_der_decoding_startEnd(c3, issuer_cert->raw.data,
				   issuer_cert->raw.size, tmpstr, &start2,
				   &end2);
	asn1_delete_structure(&c3);

	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	len2 = end2 - start2 + 1;

	/* The error code returned does not really matter
	 * here.
	 */
	if (len1 != len2) {
		gnutls_assert();
		return GNUTLS_E_UNKNOWN_ERROR;
	}
	if (memcmp(&issuer_cert->raw.data[start2],
		   &cert->raw.data[start1], len1) != 0) {
		gnutls_assert();
		return GNUTLS_E_UNKNOWN_ERROR;
	}

	/* they match */
	return 0;

}

static gnutls_cert *find_issuer(gnutls_cert * cert,
				gnutls_cert * trusted_cas, int tcas_size)
{
	int i;

	/* this is serial search. 
	 */

	for (i = 0; i < tcas_size; i++) {
		if (compare_dn(cert, &trusted_cas[i]) == 0)
			return &trusted_cas[i];
	}

	gnutls_assert();
	return NULL;
}

/* ret_trust is the value to return when the certificate chain is ok
 * ret_else is the value to return otherwise.
 */
int gnutls_verify_certificate2(gnutls_cert * cert,
			       gnutls_cert * trusted_cas, int tcas_size,
			       void *CRLs, int crls_size, int ret_trust,
			       int ret_else)
{
/* CRL is ignored for now */

	gnutls_cert *issuer;
	int ret;

	if (tcas_size >= 1)
		issuer = find_issuer(cert, trusted_cas, tcas_size);
	else {
		gnutls_assert();
		return ret_else;
	}

	/* issuer is not in trusted certificate
	 * authorities.
	 */
	if (issuer == NULL) {
		gnutls_assert();
		return ret_else;
	}

	ret = check_if_ca(cert, issuer);
	if (ret != 0) {
		gnutls_assert();
		return ret_else | GNUTLS_CERT_INVALID;
	}

	ret = gnutls_x509_verify_signature(cert, issuer);
	if (ret != 0) {
		gnutls_assert();
		return ret_else | GNUTLS_CERT_INVALID;
	}

	/* FIXME: Check CRL --not done yet.
	 */


	return ret_trust;
}

/* The algorithm used is:
 * 1. Check the certificate chain given by the peer, if it is ok.
 * 2. If any certificate in the chain are revoked, not
 *    valid, or they are not CAs then the certificate is invalid.
 * 3. If 1 is ok, then find a certificate in the trusted CAs file
 *    that has the DN of the issuer field in the last certificate
 *    in the peer's certificate chain.
 * 4. If it does exist then verify it. If verification is ok then
 *    it is trusted. Otherwise it is just valid (but not trusted).
 */
/* This function verifies a X.509 certificate list. The certificate list should
 * lead to a trusted CA in order to be trusted.
 */
int _gnutls_x509_verify_certificate(gnutls_cert * certificate_list,
				    int clist_size,
				    gnutls_cert * trusted_cas,
				    int tcas_size, void *CRLs,
				    int crls_size)
{
	int i = 0, ret;
	CertificateStatus status = 0;

	if (clist_size == 0) {
		return GNUTLS_E_NO_CERTIFICATE_FOUND;
	}

	/* Verify the certificate path */
	for (i = 0; i < clist_size; i++) {
		if (i + 1 >= clist_size)
			break;

		if ((ret =
		     gnutls_verify_certificate2(&certificate_list[i],
						&certificate_list[i + 1],
						1, NULL, 0, 0,
						GNUTLS_CERT_INVALID)) !=
		    0) {
			/*
			   * We only accept the first certificate to be
			   * expired, revoked etc. If any of the certificates in the
			   * certificate chain is expired then the certificate
			   * is not valid.
			 */
			if (ret > 0) {
				gnutls_assert();
				status |= ret;
			} else {
				gnutls_assert();
				return ret;
			}
		}
	}

	if (status > 0) { /* If there is any problem in the
			   * certificate chain then mark as not trusted
			   * and return immediately.
			   */
		return (status | GNUTLS_CERT_NOT_TRUSTED);
	}
	
	/* Now verify the last certificate in the certificate path
	 * against the trusted CA certificate list.
	 *
	 * If no CAs are present returns NOT_TRUSTED. Thus works
	 * in self signed etc certificates.
	 */
	ret =
	    gnutls_verify_certificate2(&certificate_list[i], trusted_cas,
				       tcas_size, CRLs, crls_size, 0,
				       GNUTLS_CERT_NOT_TRUSTED);

	if (ret > 0) {
		/* if the last certificate in the certificate
		 * list is invalid, then the certificate is not
		 * trusted.
		 */
		gnutls_assert();
		status |= ret;
	}

	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	/* if we got here, then it's trusted.
	 */
	return status;
}
