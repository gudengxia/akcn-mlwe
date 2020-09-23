/*
* Copyright 2002-2016 The OpenSSL Project Authors. All Rights Reserved.
*
* Licensed under the Apache License 2.0 (the "License").  You may not use
* this file except in compliance with the License.  You can obtain a copy
* in the file LICENSE in the source distribution or at
* https://www.openssl.org/source/license.html
*/

#ifndef AES_ECB_H
# define AES_ECB_H

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <stddef.h>

# if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_AMD64) || defined(_M_X64))
#  define SWAP(x) (_lrotl(x, 8) & 0x00ff00ff | _lrotr(x, 8) & 0xff00ff00)
#  define GETU32(p) SWAP(*((u32 *)(p)))
#  define PUTU32(ct, st) { *((u32 *)(ct)) = SWAP((st)); }
# else
#  define GETU32(pt) (((u32)(pt)[0] << 24) ^ ((u32)(pt)[1] << 16) ^ ((u32)(pt)[2] <<  8) ^ ((u32)(pt)[3]))
#  define PUTU32(ct, st) { (ct)[0] = (u8)((st) >> 24); (ct)[1] = (u8)((st) >> 16); (ct)[2] = (u8)((st) >>  8); (ct)[3] = (u8)(st); }
# endif

# ifdef AES_LONG
typedef unsigned long u32;
# else
typedef unsigned int u32;
# endif
typedef unsigned short u16;
typedef unsigned char u8;

# define MAXKC   (256/32)
# define MAXKB   (256/8)
# define MAXNR   14

/* This controls loop-unrolling in aes_core.c */
//# undef FULL_UNROLL
#define FULL_UNROLL


# define AES_ENCRYPT     1
# define AES_DECRYPT     0

/*
* Because array size can't be a const in C, the following two are macros.
* Both sizes are in bytes.
*/
# define AES_MAXNR 14
# define AES_BLOCK_SIZE 16

struct aes_key_st {
# ifdef AES_LONG
	unsigned long rd_key[4 * (AES_MAXNR + 1)];
# else
	unsigned int rd_key[4 * (AES_MAXNR + 1)];
# endif
	int rounds;
};
typedef struct aes_key_st AES_KEY;

int AES_set_encrypt_key(const unsigned char *userKey, const int bits,
	AES_KEY *key);
int AES_set_decrypt_key(const unsigned char *userKey, const int bits,
	AES_KEY *key);

void AES_encrypt(const unsigned char *in, unsigned char *out,
	const AES_KEY *key);
void AES_decrypt(const unsigned char *in, unsigned char *out,
	const AES_KEY *key);

#endif                          /* !AES_ECB_H */