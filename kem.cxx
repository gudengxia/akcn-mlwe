#include "api.h"
#include "rng.h"
#include "fips202.h"
#include "params.h"
#include "verify.h"
#include "indcpa.h"

/*************************************************
* Name:        crypto_kem_keypair
*
* Description: Generates public and private key
*              for CCA-secure AKCN key encapsulation mechanism
*
* Arguments:   - unsigned char *pk: pointer to output public key (an already allocated array of CRYPTO_PUBLICKEYBYTES bytes)
*              - unsigned char *sk: pointer to output private key (an already allocated array of CRYPTO_SECRETKEYBYTES bytes)
*
* Returns 0 (success)
**************************************************/
int crypto_kem_keypair(unsigned char *pk, unsigned char *sk)
{
  size_t i;
  indcpa_keypair(pk, sk);
  for(i=0;i<AKCN_INDCPA_PUBLICKEYBYTES;i++)
    sk[i+AKCN_INDCPA_SECRETKEYBYTES] = pk[i];
  sha3_256(sk+AKCN_SECRETKEYBYTES-2*AKCN_SYMBYTES,pk,AKCN_PUBLICKEYBYTES);
  randombytes(sk+AKCN_SECRETKEYBYTES-AKCN_SYMBYTES,AKCN_SYMBYTES);         /* Value z for pseudo-random output on reject */
  return 0;
}

/*************************************************
* Name:        crypto_kem_enc
*
* Description: Generates cipher text and shared
*              secret for given public key
*
* Arguments:   - unsigned char *ct:       pointer to output cipher text (an already allocated array of CRYPTO_CIPHERTEXTBYTES bytes)
*              - unsigned char *ss:       pointer to output shared secret (an already allocated array of CRYPTO_BYTES bytes)
*              - const unsigned char *pk: pointer to input public key (an already allocated array of CRYPTO_PUBLICKEYBYTES bytes)
*
* Returns 0 (success)
**************************************************/
int crypto_kem_enc(unsigned char *ct, unsigned char *ss, const unsigned char *pk)
{
  unsigned char  kr[2*AKCN_SYMBYTES];                                        /* Will contain key, coins */
  unsigned char buf[2*AKCN_SYMBYTES];

  randombytes(buf, AKCN_SYMBYTES);
  sha3_256(buf,buf,AKCN_SYMBYTES);                                           /* Don't release system RNG output */

  sha3_256(buf+AKCN_SYMBYTES, pk, AKCN_PUBLICKEYBYTES);                     /* Multitarget countermeasure for coins + contributory KEM */
  sha3_512(kr, buf, 2*AKCN_SYMBYTES);

  indcpa_enc(ct, buf, pk, kr+AKCN_SYMBYTES);                                 /* coins are in kr+AKCN_SYMBYTES */

  sha3_256(kr+AKCN_SYMBYTES, ct, AKCN_CIPHERTEXTBYTES);                     /* overwrite coins in kr with H(c) */
  sha3_256(ss, kr, 2*AKCN_SYMBYTES);                                         /* hash concatenation of pre-k and H(c) to k */
  return 0;
}

/*************************************************
* Name:        crypto_kem_dec
*
* Description: Generates shared secret for given
*              cipher text and private key
*
* Arguments:   - unsigned char *ss:       pointer to output shared secret (an already allocated array of CRYPTO_BYTES bytes)
*              - const unsigned char *ct: pointer to input cipher text (an already allocated array of CRYPTO_CIPHERTEXTBYTES bytes)
*              - const unsigned char *sk: pointer to input private key (an already allocated array of CRYPTO_SECRETKEYBYTES bytes)
*
* Returns 0 for sucess or -1 for failure
*
* On failure, ss will contain a randomized value.
**************************************************/
int crypto_kem_dec(unsigned char *ss, const unsigned char *ct, const unsigned char *sk)
{
  size_t i;
  int fail;
  unsigned char cmp[AKCN_CIPHERTEXTBYTES];
  unsigned char buf[2*AKCN_SYMBYTES];
  unsigned char kr[2*AKCN_SYMBYTES];                                         /* Will contain key, coins, qrom-hash */
  const unsigned char *pk = sk+AKCN_INDCPA_SECRETKEYBYTES;

  indcpa_dec(buf, ct, sk);

  for(i=0;i<AKCN_SYMBYTES;i++)                                               /* Multitarget countermeasure for coins + contributory KEM */
    buf[AKCN_SYMBYTES+i] = sk[AKCN_SECRETKEYBYTES-2*AKCN_SYMBYTES+i];      /* Save hash by storing H(pk) in sk */
  sha3_512(kr, buf, 2*AKCN_SYMBYTES);

  indcpa_enc(cmp, buf, pk, kr+AKCN_SYMBYTES);                                /* coins are in kr+AKCN_SYMBYTES */

  fail = verify(ct, cmp, AKCN_CIPHERTEXTBYTES);

  sha3_256(kr+AKCN_SYMBYTES, ct, AKCN_CIPHERTEXTBYTES);                     /* overwrite coins in kr with H(c)  */

  cmov(kr, sk+AKCN_SECRETKEYBYTES-AKCN_SYMBYTES, AKCN_SYMBYTES, fail);     /* Overwrite pre-k with z on re-encryption failure */

  sha3_256(ss, kr, 2*AKCN_SYMBYTES);                                         /* hash concatenation of pre-k and H(c) to k */

  return -fail;
}
