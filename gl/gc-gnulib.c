/* gc-gl-common.c --- Common gnulib internal crypto interface functions
 * Copyright (C) 2002, 2003, 2004, 2005  Simon Josefsson
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1, or (at your
 * option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this file; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

/* Note: This file is only built if GC uses internal functions. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Get prototype. */
#include <gc.h>

#include <stdlib.h>
#include <string.h>

/* For randomize. */
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

/* Hashes. */
#ifdef GC_USE_MD4
# include "md4.h"
#endif
#ifdef GC_USE_MD5
# include "md5.h"
#endif
#ifdef GC_USE_SHA1
# include "sha1.h"
#endif
#ifdef GC_USE_HMAC_MD5
# include "hmac.h"
#endif

/* Ciphers. */
#ifdef GC_USE_ARCFOUR
# include "arcfour.h"
#endif
#ifdef GC_USE_ARCTWO
# include "arctwo.h"
#endif
#ifdef GC_USE_RIJNDAEL
# include "rijndael-api-fst.h"
#endif

Gc_rc
gc_init (void)
{
  return GC_OK;
}

void
gc_done (void)
{
  return;
}

/* Randomness. */

static Gc_rc
randomize (int level, char *data, size_t datalen)
{
  int fd;
  const char *device;
  size_t len = 0;
  int rc;

  switch (level)
    {
    case 0:
      device = NAME_OF_NONCE_DEVICE;
      break;

    case 1:
      device = NAME_OF_PSEUDO_RANDOM_DEVICE;
      break;

    default:
      device = NAME_OF_RANDOM_DEVICE;
      break;
    }

  fd = open (device, O_RDONLY);
  if (fd < 0)
    return GC_RANDOM_ERROR;

  do
    {
      ssize_t tmp;

      tmp = read (fd, data, datalen);

      if (tmp < 0)
	{
	  int save_errno = errno;
	  close (fd);
	  errno = save_errno;
	  return GC_RANDOM_ERROR;
	}

      len += tmp;
    }
  while (len < datalen);

  rc = close (fd);
  if (rc < 0)
    return GC_RANDOM_ERROR;

  return GC_OK;
}

Gc_rc
gc_nonce (char *data, size_t datalen)
{
  return randomize (0, data, datalen);
}

Gc_rc
gc_pseudo_random (char *data, size_t datalen)
{
  return randomize (1, data, datalen);
}

Gc_rc
gc_random (char *data, size_t datalen)
{
  return randomize (2, data, datalen);
}

/* Memory allocation. */

void
gc_set_allocators (gc_malloc_t func_malloc,
		   gc_malloc_t secure_malloc,
		   gc_secure_check_t secure_check,
		   gc_realloc_t func_realloc, gc_free_t func_free)
{
  return;
}
/* Ciphers. */

typedef struct _gc_cipher_ctx {
  Gc_cipher alg;
  Gc_cipher_mode mode;
#ifdef GC_USE_ARCTWO
  arctwo_context arctwoContext;
#endif
#ifdef GC_USE_ARCFOUR
  arcfour_context arcfourContext;
#endif
#ifdef GC_USE_RIJNDAEL
  rijndaelKeyInstance aesEncKey;
  rijndaelKeyInstance aesDecKey;
  rijndaelCipherInstance aesContext;
#endif
} _gc_cipher_ctx;

Gc_rc
gc_cipher_open (Gc_cipher alg, Gc_cipher_mode mode,
		gc_cipher_handle * outhandle)
{
  _gc_cipher_ctx *ctx;
  Gc_rc rc = GC_OK;

  ctx = calloc (sizeof (*ctx), 1);

  ctx->alg = alg;
  ctx->mode = mode;

  switch (alg)
    {
#ifdef GC_USE_ARCTWO
    case GC_ARCTWO40:
      switch (mode)
	{
	case GC_ECB:
	  break;

	default:
	  rc = GC_INVALID_CIPHER;
	}
      break;
#endif

#ifdef GC_USE_ARCFOUR
    case GC_ARCFOUR128:
    case GC_ARCFOUR40:
      switch (mode)
	{
	case GC_STREAM:
	  break;

	default:
	  rc = GC_INVALID_CIPHER;
	}
      break;
#endif

#ifdef GC_USE_RIJNDAEL
    case GC_AES128:
    case GC_AES192:
    case GC_AES256:
      switch (mode)
	{
	case GC_ECB:
	case GC_CBC:
	  break;

	default:
	  rc = GC_INVALID_CIPHER;
	}
      break;
#endif

    default:
      rc = GC_INVALID_CIPHER;
    }

  if (rc == GC_OK)
    *outhandle = ctx;
  else
    free (ctx);

  return rc;
}

Gc_rc
gc_cipher_setkey (gc_cipher_handle handle, size_t keylen, const char *key)
{
  _gc_cipher_ctx *ctx = handle;

  switch (ctx->alg)
    {
#ifdef GC_USE_ARCTWO
    case GC_ARCTWO40:
      arctwo_setkey (&ctx->arctwoContext, keylen, key);
      break;
#endif

#ifdef GC_USE_ARCFOUR
    case GC_ARCFOUR128:
    case GC_ARCFOUR40:
      arcfour_setkey (&ctx->arcfourContext, key, keylen);
      break;
#endif

#ifdef GC_USE_RIJNDAEL
    case GC_AES128:
    case GC_AES192:
    case GC_AES256:
      {
	rijndael_rc rc;
	size_t i;
	char keyMaterial[RIJNDAEL_MAX_KEY_SIZE + 1];

	for (i = 0; i < keylen; i++)
	  sprintf (&keyMaterial[2*i], "%02x", key[i] & 0xFF);

	rc = rijndaelMakeKey (&ctx->aesEncKey, RIJNDAEL_DIR_ENCRYPT,
			      keylen * 8, keyMaterial);
	if (rc < 0)
	  return GC_INVALID_CIPHER;

	rc = rijndaelMakeKey (&ctx->aesDecKey, RIJNDAEL_DIR_DECRYPT,
			      keylen * 8, keyMaterial);
	if (rc < 0)
	  return GC_INVALID_CIPHER;

	rc = rijndaelCipherInit (&ctx->aesContext, RIJNDAEL_MODE_ECB, NULL);
	if (rc < 0)
	  return GC_INVALID_CIPHER;
      }
      break;
#endif

    default:
      return GC_INVALID_CIPHER;
    }

  return GC_OK;
}

Gc_rc
gc_cipher_setiv (gc_cipher_handle handle, size_t ivlen, const char *iv)
{
  _gc_cipher_ctx *ctx = handle;

  switch (ctx->alg)
    {
#ifdef GC_USE_RIJNDAEL
    case GC_AES128:
    case GC_AES192:
    case GC_AES256:
      switch (ctx->mode)
	{
	case GC_ECB:
	  /* Doesn't use IV. */
	  break;

	case GC_CBC:
	  {
	    rijndael_rc rc;
	    size_t i;
	    char ivMaterial[2 * RIJNDAEL_MAX_IV_SIZE + 1];

	    for (i = 0; i < ivlen; i++)
	      sprintf (&ivMaterial[2*i], "%02x", iv[i] & 0xFF);

	    rc = rijndaelCipherInit (&ctx->aesContext, RIJNDAEL_MODE_CBC,
				     ivMaterial);
	    if (rc < 0)
	      return GC_INVALID_CIPHER;
	  }
	  break;

	default:
	  return GC_INVALID_CIPHER;
	}
      break;
#endif

    default:
      return GC_INVALID_CIPHER;
    }

  return GC_OK;
}

Gc_rc
gc_cipher_encrypt_inline (gc_cipher_handle handle, size_t len, char *data)
{
  _gc_cipher_ctx *ctx = handle;

  switch (ctx->alg)
    {
#ifdef GC_USE_ARCTWO
    case GC_ARCTWO40:
      arctwo_encrypt (&ctx->arctwoContext, data, data, len);
      break;
#endif

#ifdef GC_USE_ARCFOUR
    case GC_ARCFOUR128:
    case GC_ARCFOUR40:
      arcfour_stream (&ctx->arcfourContext, data, data, len);
      break;
#endif

#ifdef GC_USE_RIJNDAEL
    case GC_AES128:
    case GC_AES192:
    case GC_AES256:
      {
	int nblocks;

	nblocks = rijndaelBlockEncrypt (&ctx->aesContext, &ctx->aesEncKey,
					data, 8 * len, data);
	if (nblocks < 0)
	  return GC_INVALID_CIPHER;
      }
      break;
#endif

    default:
      return GC_INVALID_CIPHER;
    }

  return GC_OK;
}

Gc_rc
gc_cipher_decrypt_inline (gc_cipher_handle handle, size_t len, char *data)
{
  _gc_cipher_ctx *ctx = handle;

  switch (ctx->alg)
    {
#ifdef GC_USE_ARCTWO
    case GC_ARCTWO40:
      arctwo_decrypt (&ctx->arctwoContext, data, data, len);
      break;
#endif

#ifdef GC_USE_ARCFOUR
    case GC_ARCFOUR128:
    case GC_ARCFOUR40:
      arcfour_stream (&ctx->arcfourContext, data, data, len);
      break;
#endif

#ifdef GC_USE_RIJNDAEL
    case GC_AES128:
    case GC_AES192:
    case GC_AES256:
      {
	int nblocks;

	nblocks = rijndaelBlockDecrypt (&ctx->aesContext, &ctx->aesDecKey,
					data, 8 * len, data);
	if (nblocks < 0)
	  return GC_INVALID_CIPHER;
      }
      break;
#endif

    default:
      return GC_INVALID_CIPHER;
    }

  return GC_OK;
}

Gc_rc
gc_cipher_close (gc_cipher_handle handle)
{
  _gc_cipher_ctx *ctx = handle;

  if (ctx)
    free (ctx);

  return GC_OK;
}

/* Hashes. */

Gc_rc
gc_hash_buffer (Gc_hash hash, const void *in, size_t inlen, char *resbuf)
{
  switch (hash)
    {
#ifdef GC_USE_MD4
    case GC_MD4:
      md4_buffer (in, inlen, resbuf);
      break;
#endif

#ifdef GC_USE_MD5
    case GC_MD5:
      md5_buffer (in, inlen, resbuf);
      break;
#endif

#ifdef GC_USE_SHA1
    case GC_SHA1:
      sha1_buffer (in, inlen, resbuf);
      break;
#endif

    default:
      return GC_INVALID_HASH;
    }

  return GC_OK;
}

#ifdef GC_USE_MD4
Gc_rc
gc_md4 (const void *in, size_t inlen, void *resbuf)
{
  md4_buffer (in, inlen, resbuf);
  return GC_OK;
}
#endif

#ifdef GC_USE_MD5
Gc_rc
gc_md5 (const void *in, size_t inlen, void *resbuf)
{
  md5_buffer (in, inlen, resbuf);
  return GC_OK;
}
#endif

#ifdef GC_USE_SHA1
Gc_rc
gc_sha1 (const void *in, size_t inlen, void *resbuf)
{
  sha1_buffer (in, inlen, resbuf);
  return GC_OK;
}
#endif

#ifdef GC_USE_HMAC_MD5
Gc_rc
gc_hmac_md5 (const void *key, size_t keylen,
	     const void *in, size_t inlen, char *resbuf)
{
  hmac_md5 (key, keylen, in, inlen, resbuf);
  return GC_OK;
}
#endif

#ifdef GC_USE_HMAC_SHA1
Gc_rc
gc_hmac_sha1 (const void *key, size_t keylen,
	      const void *in, size_t inlen, char *resbuf)
{
  hmac_sha1 (key, keylen, in, inlen, resbuf);
  return GC_OK;
}
#endif

#include <gcrypt.h>

Gc_rc
gc_hash_open (Gc_hash hash, Gc_hash_mode mode, gc_hash_handle * outhandle)
{
  int gcryalg, gcrymode;
  gcry_error_t err;

  switch (hash)
    {
    case GC_MD4:
      gcryalg = GCRY_MD_MD4;
      break;

    case GC_MD5:
      gcryalg = GCRY_MD_MD5;
      break;

    case GC_SHA1:
      gcryalg = GCRY_MD_SHA1;
      break;

    case GC_RMD160:
      gcryalg = GCRY_MD_RMD160;
      break;

    default:
      return GC_INVALID_HASH;
    }

  switch (mode)
    {
    case 0:
      gcrymode = 0;
      break;

    case GC_HMAC:
      gcrymode = GCRY_MD_FLAG_HMAC;
      break;

    default:
      return GC_INVALID_HASH;
    }

  err = gcry_md_open ((gcry_md_hd_t *) outhandle, gcryalg, gcrymode);
  if (gcry_err_code (err))
    return GC_INVALID_HASH;

  return GC_OK;
}

Gc_rc
gc_hash_clone (gc_hash_handle handle, gc_hash_handle * outhandle)
{
  int err;

  err = gcry_md_copy ((gcry_md_hd_t *) outhandle, (gcry_md_hd_t) handle);
  if (err)
    return GC_INVALID_HASH;

  return GC_OK;
}

size_t
gc_hash_digest_length (Gc_hash hash)
{
  int gcryalg;

  switch (hash)
    {
    case GC_MD4:
      gcryalg = GCRY_MD_MD4;
      break;

    case GC_MD5:
      gcryalg = GCRY_MD_MD5;
      break;

    case GC_SHA1:
      gcryalg = GCRY_MD_SHA1;
      break;

    case GC_RMD160:
      gcryalg = GCRY_MD_RMD160;
      break;

    default:
      return 0;
    }

  return gcry_md_get_algo_dlen (gcryalg);
}

void
gc_hash_hmac_setkey (gc_hash_handle handle, size_t len, const char *key)
{
  gcry_md_setkey ((gcry_md_hd_t) handle, key, len);
}

void
gc_hash_write (gc_hash_handle handle, size_t len, const char *data)
{
  gcry_md_write ((gcry_md_hd_t) handle, data, len);
}

const char *
gc_hash_read (gc_hash_handle handle)
{
  const char *digest;

  gcry_md_final ((gcry_md_hd_t) handle);
  digest = gcry_md_read ((gcry_md_hd_t) handle, 0);

  return digest;
}

void
gc_hash_close (gc_hash_handle handle)
{
  gcry_md_close ((gcry_md_hd_t) handle);
}
