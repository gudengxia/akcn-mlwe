#include <stdio.h>
#include "polyvec.h"
#include "fips202.h"
#include "cbd.h"
#include "reduce.h"

/*************************************************
* Name:        polyvec_compress
*
* Description: Compress and serialize vector of polynomials
*
* Arguments:   - unsigned char *r: pointer to output byte array
*              - const polyvec *a: pointer to input vector of polynomials
**************************************************/
void polyvec_compress(unsigned char *r, const polyvec *a)
{
  int i,j,k;
  uint16_t t[8];
  for(i=0;i<AKCN_K;i++)
  {
    for(j=0;j<AKCN_N/8;j++)
    {
      for(k=0;k<8;k++)
        t[k] = ((((uint32_t)freeze(a->vec[i].coeffs[8*j+k]) << 10) + AKCN_Q/2)/ AKCN_Q) & 0x3ff;

      r[10*j+ 0] =  t[0] & 0xff;
      r[10*j+ 1] = (t[0] >> 8) | ((t[1] & 0x3f) << 2);
      r[10*j+ 2] = (t[1] >> 6) | ((t[2] & 0x0f) << 4);
      r[10*j+ 3] = (t[2] >> 4) | ((t[3] & 0x03) << 6);
      r[10*j+ 4] =  t[3] >> 2;
      r[10*j+ 5] =  t[4] & 0xff;
      r[10*j+ 6] = (t[4] >> 8) | ((t[5] & 0x3f) << 2);
      r[10*j+ 7] = (t[5] >> 6) | ((t[6] & 0x0f) << 4);
      r[10*j+ 8] = (t[6] >> 4) | ((t[7] & 0x03) << 6);
      r[10*j+ 9] =  t[7] >> 2;
    }
    r += 320;
  }
}

/*************************************************
* Name:        polyvec_decompress
*
* Description: De-serialize and decompress vector of polynomials;
*              approximate inverse of polyvec_compress
*
* Arguments:   - polyvec *r:       pointer to output vector of polynomials
*              - unsigned char *a: pointer to input byte array
**************************************************/
void polyvec_decompress(polyvec *r, const unsigned char *a)
{
  int i,j;
  for(i=0;i<AKCN_K;i++)
  {
    for(j=0;j<AKCN_N/8;j++)
    {
      r->vec[i].coeffs[8*j+0] =  (((a[10*j+ 0]       | (((uint32_t)a[10*j+ 1] & 0x03) << 8)) * AKCN_Q) + 1024) >> 10;
      r->vec[i].coeffs[8*j+1] = ((((a[10*j+ 1] >> 2) | (((uint32_t)a[10*j+ 2] & 0x0f) << 6)) * AKCN_Q) + 1024) >> 10;
      r->vec[i].coeffs[8*j+2] = ((((a[10*j+ 2] >> 4) | (((uint32_t)a[10*j+ 3] & 0x3f) << 4)) * AKCN_Q) + 1024) >> 10;
      r->vec[i].coeffs[8*j+3] = ((((a[10*j+ 3] >> 6) | (((uint32_t)a[10*j+ 4]       ) << 2)) * AKCN_Q) + 1024) >> 10;
      r->vec[i].coeffs[8*j+4] =  (((a[10*j+ 5]       | (((uint32_t)a[10*j+ 6] & 0x03) << 8)) * AKCN_Q) + 1024) >> 10;
      r->vec[i].coeffs[8*j+5] = ((((a[10*j+ 6] >> 2) | (((uint32_t)a[10*j+ 7] & 0x0f) << 6)) * AKCN_Q) + 1024) >> 10;
      r->vec[i].coeffs[8*j+6] = ((((a[10*j+ 7] >> 4) | (((uint32_t)a[10*j+ 8] & 0x3f) << 4)) * AKCN_Q) + 1024) >> 10;
      r->vec[i].coeffs[8*j+7] = ((((a[10*j+ 8] >> 6) | (((uint32_t)a[10*j+ 9]       ) << 2)) * AKCN_Q) + 1024) >> 10;
    }
    a += 320;
  }
}

/*************************************************
* Name:        polyvec_tobytes
*
* Description: Serialize vector of polynomials
*
* Arguments:   - unsigned char *r: pointer to output byte array
*              - const polyvec *a: pointer to input vector of polynomials
**************************************************/
void polyvec_tobytes(unsigned char *r, const polyvec *a)
{
  int i;
  for(i=0;i<AKCN_K;i++)
    poly_tobytes(r+i*AKCN_POLYBYTES, &a->vec[i]);
}

/*************************************************
* Name:        polyvec_frombytes
*
* Description: De-serialize vector of polynomials;
*              inverse of polyvec_tobytes
*
* Arguments:   - unsigned char *r: pointer to output byte array
*              - const polyvec *a: pointer to input vector of polynomials
**************************************************/
void polyvec_frombytes(polyvec *r, const unsigned char *a)
{
  int i;
  for(i=0;i<AKCN_K;i++)
    poly_frombytes(&r->vec[i], a+i*AKCN_POLYBYTES);
}

/*************************************************
* Name:        polyvec_ntt
*
* Description: Apply forward NTT to all elements of a vector of polynomials
*
* Arguments:   - polyvec *r: pointer to in/output vector of polynomials
**************************************************/
void polyvec_ntt(polyvec *r)
{
  int i;
  for(i=0;i<AKCN_K;i++)
    poly_ntt(&r->vec[i]);
}

/*************************************************
* Name:        polyvec_invntt
*
* Description: Apply inverse NTT to all elements of a vector of polynomials
*
* Arguments:   - polyvec *r: pointer to in/output vector of polynomials
**************************************************/
void polyvec_invntt(polyvec *r)
{
  int i;
  for(i=0;i<AKCN_K;i++)
    poly_invntt(&r->vec[i]);
}

/*************************************************
* Name:        polyvec_pointwise_acc
*
* Description: Pointwise multiply elements of a and b and accumulate into r
*
* Arguments: - poly *r:          pointer to output polynomial
*            - const polyvec *a: pointer to first input vector of polynomials
*            - const polyvec *b: pointer to second input vector of polynomials
**************************************************/
void polyvec_pointwise_acc(poly *r, const polyvec *a, const polyvec *b)
{
  int i,j;
  uint16_t t;
  for(j=0;j<AKCN_N;j++)
  {
    t = montgomery_reduce(4613* (uint32_t)b->vec[0].coeffs[j]); // 4613 = 2^{2*18} % q
    r->coeffs[j] = montgomery_reduce(a->vec[0].coeffs[j] * t);
    for(i=1;i<AKCN_K;i++)
    {
      t = montgomery_reduce(4613* (uint32_t)b->vec[i].coeffs[j]);
      r->coeffs[j] += montgomery_reduce(a->vec[i].coeffs[j] * t);
    }
    r->coeffs[j] = barrett_reduce(r->coeffs[j]);
  }
}

/*************************************************
* Name:        polyvec_add
*
* Description: Add vectors of polynomials
*
* Arguments: - polyvec *r:       pointer to output vector of polynomials
*            - const polyvec *a: pointer to first input vector of polynomials
*            - const polyvec *b: pointer to second input vector of polynomials
**************************************************/
void polyvec_add(polyvec *r, const polyvec *a, const polyvec *b)
{
  int i;
  for(i=0;i<AKCN_K;i++)
    poly_add(&r->vec[i], &a->vec[i], &b->vec[i]);

}
