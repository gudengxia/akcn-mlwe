#ifndef API_H
#define API_H

#include "params.h"

#define CRYPTO_SECRETKEYBYTES  AKCN_SECRETKEYBYTES
#define CRYPTO_PUBLICKEYBYTES  AKCN_PUBLICKEYBYTES
#define CRYPTO_CIPHERTEXTBYTES AKCN_CIPHERTEXTBYTES
#define CRYPTO_BYTES           AKCN_SYMBYTES

#define CRYPTO_ALGNAME "AKCN-MLWE"

int crypto_kem_keypair(unsigned char *pk, unsigned char *sk);

int crypto_kem_enc(unsigned char *ct, unsigned char *ss, const unsigned char *pk);

int crypto_kem_dec(unsigned char *ss, const unsigned char *ct, const unsigned char *sk);


#endif
