//
//  rng.c
//
//  Created by Bassham, Lawrence E (Fed) on 8/29/17.
//  Copyright © 2017 Bassham, Lawrence E (Fed). All rights reserved.
//

#include <string.h>
#include "aes.h"
#include "fips202.h"
// #include "Alg.h"
#include "rng.h"


#define RNG_SUCCESS      0
#define RNG_BAD_MAXLEN  -1
#define RNG_BAD_OUTBUF  -2
#define RNG_BAD_REQ_LEN -3
#define RNG_SEED_BYTES 48

typedef struct {
	unsigned char   Key[32];
	unsigned char   V[16];
	int             reseed_counter;
} AES256_CTR_DRBG_struct;

/*
void AES256_ECB(unsigned char *key, unsigned char *ctr, unsigned char *buffer);
void
AES256_CTR_DRBG_Update(unsigned char *provided_data,
	unsigned char *Key,
	unsigned char *V);
*/
// This uses AES tailored from openSSL library
//    key - 256-bit AES key
//    ctr - a 128-bit plaintext value
//    buffer - a 128-bit ciphertext value
void AES256_ECB(unsigned char *key, unsigned char *ctr, unsigned char *buffer)
{
	AES_KEY ek;
	AES_set_encrypt_key(key, 256, &ek);
	AES_encrypt(ctr, buffer, &ek);
}

void AES256_CTR_DRBG_Update(unsigned char *provided_data,
	unsigned char *Key,
	unsigned char *V)
{
	unsigned char   temp[48];

	for (int i = 0; i < 3; i++) {
		//increment V
		for (int j = 15; j >= 0; j--) {
			if (V[j] == 0xff)
				V[j] = 0x00;
			else {
				V[j]++;
				break;
			}
		}

		AES256_ECB(Key, V, temp + 16 * i);
	}
	if (provided_data != NULL)
		for (int i = 0; i < 48; i++)
			temp[i] ^= provided_data[i];
	memcpy(Key, temp, 32);
	memcpy(V, temp + 32, 16);
}

AES256_CTR_DRBG_struct  DRBG_ctx;

unsigned long long rand_get_sd_byts()
{
	return RNG_SEED_BYTES;
}

int rand_init(unsigned char * s, unsigned long long s_byts)
{
	unsigned char   seed_material[48] = { 0 };

	if (s_byts > 48)
		sha3_384(seed_material, s, s_byts);//the extra entropy is also used in compression
	else
		memcpy(seed_material, s, s_byts);

	memset(DRBG_ctx.Key, 0x00, 32);
	memset(DRBG_ctx.V, 0x00, 16);
	AES256_CTR_DRBG_Update(seed_material, DRBG_ctx.Key, DRBG_ctx.V);
	DRBG_ctx.reseed_counter = 1;

	return 0;
}
int rand_byts(unsigned long long r_byts, unsigned char * r)
{

	unsigned char   block[16];
	int             i = 0;

	while (r_byts > 0) {
		//increment V
		for (int j = 15; j >= 0; j--) {
			if (DRBG_ctx.V[j] == 0xff)
				DRBG_ctx.V[j] = 0x00;
			else {
				DRBG_ctx.V[j]++;
				break;
			}
		}
		AES256_ECB(DRBG_ctx.Key, DRBG_ctx.V, block);
		if (r_byts > 15) {
			memcpy(r + i, block, 16);
			i += 16;
			r_byts -= 16;
		}
		else {
			memcpy(r + i, block, r_byts);
			r_byts = 0;
		}
	}
	AES256_CTR_DRBG_Update(NULL, DRBG_ctx.Key, DRBG_ctx.V);
	DRBG_ctx.reseed_counter++;

	return 0;

}


int randombytes(unsigned char * r, unsigned long long r_byts)
{
	return rand_byts(r_byts, r);
}
