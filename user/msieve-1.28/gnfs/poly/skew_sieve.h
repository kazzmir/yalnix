/*--------------------------------------------------------------------
This source distribution is placed in the public domain by its author,
Jason Papadopoulos. You may use it for any purpose, free of charge,
without having to notify anyone. I disclaim any responsibility for any
errors.

Optionally, please be nice and tell me if you find this source to be
useful. Again optionally, if you add to the functionality present here
please consider making those additions public too, so that others may 
benefit from your work.	
       				   --jasonp@boo.net 10/3/07
--------------------------------------------------------------------*/

#ifndef _ALG_SIEVE_H_
#define _ALG_SIEVE_H_

#include <util.h>

#ifdef __cplusplus
extern "C" {
#endif

/* main structure controlling leading algebraic 
   coefficient sieving */

typedef struct {
	uint32 *strides;
	uint32 *roots;
	uint8 *logs;
	uint8 cutoff;
	uint8 *sieve;
	uint64 base;
	uint32 curr_offset;
} alg_sieve_t;

void alg_sieve_init(uint64 base, alg_sieve_t *s);

void alg_sieve_free(alg_sieve_t *s);

uint64 alg_sieve_next(alg_sieve_t *s);

#define MAX_RAT_FACTOR_SIZE 1500

typedef struct {
	uint32 num_factors;
	uint16 *factor_list;
} rat_sieve_t;

void rat_sieve_init(rat_sieve_t *r, uint16 *other_factors,
			uint32 num_other_factors);

void rat_sieve_free(rat_sieve_t *r);

#ifdef __cplusplus
}
#endif

#endif /* !_ALG_SIEVE_H_ */
