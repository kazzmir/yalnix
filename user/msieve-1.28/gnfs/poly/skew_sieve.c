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

#include "skew_sieve.h"

/* Leading coefficients of the algebraic polynomial 
   are found by sieving */

#define ALG_SIEVE_SIZE 4096

/* we sieve with powers of primes. Each power gets 
   the same log value, and log values stack up 
   at locations divisible by powers of a prime */

static const uint8 alg_sieve_p[] = 
	{2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 
	 5, 5, 5, 5, 7, 7, 7, 11, 11, 13, 13, 17, 17, 
	 19, 19, 23, 23, 29, 29, 31, 31};

/* the sieving stride used in each prime power */

static const uint16 alg_sieve_powers[] = 
	{2, 4, 8, 16, 32, 64, 128, 256, 512, 
	 3, 9, 27, 81, 243, 729, 5, 25, 125, 625, 
	 7, 49, 343, 11, 121, 13, 169, 17, 289, 
	 19, 361, 23, 529, 29, 841, 31, 961};

#define NUM_ALG_SIEVE_ENTRIES sizeof(alg_sieve_p)

/* scale factor for logs of sieve primes */

#define ALG_SIEVE_LOG_SCALE 6.0

/*------------------------------------------------------------------*/
static void alg_sieve_next_block(alg_sieve_t *s) {

	uint32 i;
	uint8 *sieve = s->sieve;

	/* fill the next sieve block. Sieve values begin
	   with the target log value and are decremented
	   as primes update the block. Values that meet
	   the cutoff get their sign bit set, which is
	   easy to check in parallel */

	memset(sieve, s->cutoff, (size_t)ALG_SIEVE_SIZE);
	for (i = 0; i < NUM_ALG_SIEVE_ENTRIES; i++) {
		uint32 root = s->roots[i];
		uint32 stride = s->strides[i];
		uint8 logp = s->logs[i];

		while (root < ALG_SIEVE_SIZE) {
			sieve[root] -= logp;
			root += stride;
		}
		s->roots[i] = root - ALG_SIEVE_SIZE;
	}
}

/*------------------------------------------------------------------*/
void alg_sieve_init(uint64 base, alg_sieve_t *s) {

	/* Sieving will begin from 'base' and proceed in 
	   blocks of size ALG_SIEVE_SIZE */

	uint32 i;
	uint32 num_entries = NUM_ALG_SIEVE_ENTRIES;

	s->sieve = (uint8 *)aligned_malloc(ALG_SIEVE_SIZE, 16);
	if ( ! s->sieve ){
		TtyPrintf( 0, "no free memory\n" );
		exit( -1 );
	}
	s->strides = (uint32 *)malloc(num_entries * sizeof(uint32));
	if ( ! s->strides ){
		TtyPrintf( 0, "no free memory\n" );
		exit( -1 );
	}
	s->roots = (uint32 *)malloc(num_entries * sizeof(uint32));
	if ( ! s->roots ){
		TtyPrintf( 0, "no free memory\n" );
		exit( -1 );
	}
	s->logs = (uint8 *)malloc(num_entries * sizeof(uint8));
	if ( ! s->logs ){
		TtyPrintf( 0, "no free memory\n" );
		exit( -1 );
	}
	s->cutoff = (uint8)(ALG_SIEVE_LOG_SCALE * 
				log((double)base) / M_LN2 + 0.5);
	s->base = base;
	s->curr_offset = 0;
	
	for (i = 0; i < num_entries; i++) {
		uint32 root;
		s->strides[i] = alg_sieve_powers[i];
		s->logs[i] = (uint8)(ALG_SIEVE_LOG_SCALE * 
				log((double)alg_sieve_p[i]) / M_LN2 + 0.5);
		root = base % s->strides[i];
		if (root > 0)
			root = s->strides[i] - root;
		s->roots[i] = root;
	}

	if (s->cutoff > s->logs[i-1])
		s->cutoff -= s->logs[i-1];
	else
		s->cutoff = 0;

	alg_sieve_next_block(s);
}

/*------------------------------------------------------------------*/
void alg_sieve_free(alg_sieve_t *s) {
	aligned_free(s->sieve);
	free(s->strides);
	free(s->roots);
	free(s->logs);
}

/*------------------------------------------------------------------*/
static void alg_sieve_update_logs(uint64 new_base, alg_sieve_t *s) {

	/* as sieve values grow, it becomes harder and harder
	   to find smooth values. Update the sieving cutoff 
	   to reflect this */

	s->cutoff = (uint8)(ALG_SIEVE_LOG_SCALE * 
				log((double)new_base) / M_LN2 + 0.5);

	if (s->cutoff > s->logs[NUM_ALG_SIEVE_ENTRIES-1])
		s->cutoff -= s->logs[NUM_ALG_SIEVE_ENTRIES-1];
	else
		s->cutoff = 0;
}

/*------------------------------------------------------------------*/
#define PACKED_SIEVE_MASK ((uint64)0x80808080 << 32 | 0x80808080)

uint64 alg_sieve_next(alg_sieve_t *s) {

	uint32 i, j;
	uint64 *packed_sieve = (uint64 *)(s->sieve);
	uint32 curr_offset = s->curr_offset;

	while (1) {
		for (i = curr_offset / 8; i < ALG_SIEVE_SIZE/8; i++) {
			if ((packed_sieve[i] & PACKED_SIEVE_MASK) == (uint64)0)
				continue;
	
			for (j = 0; j < 8; j++) {
				uint32 off = 8 * i + j;
				if (off >= curr_offset &&
				    s->sieve[off] > s->cutoff) {
					s->curr_offset = off + 1;
					return s->base + off;
				}
			}
		}
		alg_sieve_next_block(s);
		s->base += ALG_SIEVE_SIZE;
		alg_sieve_update_logs(s->base, s);
		curr_offset = 0;
	}

	return 0;	/* should never happen */
}

/*------------------------------------------------------------------*/
void rat_sieve_init(rat_sieve_t *r, uint16 *other_factors,
			uint32 num_other_factors) {

	uint32 i, j;
	uint8 *sieve;

	r->factor_list = (uint16 *)malloc((MAX_RAT_FACTOR_SIZE / 3) * 
						sizeof(uint16));
	if ( ! r->factor_list ){
		TtyPrintf( 0, "no free memory\n" );
		exit( -1 );
	}
	sieve = (uint8 *)calloc((size_t)(MAX_RAT_FACTOR_SIZE / 2 + 7) / 8, 
						sizeof(uint8));
	if ( ! sieve ){
		TtyPrintf( 0, "no free memory\n" );
		exit( -1 );
	}

	for (i = 0; i < num_other_factors + 2; i++) {

		uint32 p;

		if (i == num_other_factors)
			p = 3;
		else if (i == num_other_factors + 1)
			p = 5;
		else
			p = other_factors[i];

		for (j = p / 2; j < MAX_RAT_FACTOR_SIZE / 2; j += p)
			sieve[j / 8] |= 1 << (j % 8);
	}

	for (i = 3, j = 0; i < MAX_RAT_FACTOR_SIZE / 2; i++) {

		if (!(sieve[i / 8] & (1 << (i % 8))))
			r->factor_list[j++] = 2 * i + 1;
	}

	r->num_factors = j;
	free(sieve);
}

/*------------------------------------------------------------------*/
void rat_sieve_free(rat_sieve_t *r) {

	free(r->factor_list);
}
