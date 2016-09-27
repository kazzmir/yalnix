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

#ifndef _POLY_H_
#define _POLY_H_

#include <common.h>
#include <dd.h>
#include <integrate.h>
#include "gnfs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* used if polynomials will ever be generated in parallel */
#define POLY_HEAP_SIZE 5

/* when analyzing a polynomial's root properties, the
   bound on factor base primes that are checked */
#define PRIME_BOUND 3000

typedef struct {
	mp_poly_t rpoly;
	mp_poly_t apoly;
	double size_score;
	double root_score;
	double combined_score;
} poly_select_t;

/* when analyzing a polynomial's root properties, the
   number of independent sample values that are used to
   get an estimate of the contribution of small factor
   base primes to random sieve values */

#define NUM_POLY_SAMPLES 3000

/* main structure for poly selection */

typedef struct {
	prime_list_t prime_list;

	poly_select_t *heap[POLY_HEAP_SIZE];
	uint32 heap_num_filled;

	abpair_t *samples;

	integrate_t integ_aux;
} poly_config_t;

void poly_config_init(msieve_obj *obj, poly_config_t *config);
void poly_config_free(poly_config_t *config);

/* main routines */

void find_poly_noskew(msieve_obj *obj, mp_t *n,
			poly_config_t *config,
			uint32 deadline);

void find_poly_skew(msieve_obj *obj, mp_t *n,
			poly_config_t *config,
			uint32 deadline);

void analyze_poly(poly_config_t *config,
			mp_poly_t *rpoly, 
			mp_poly_t *apoly);

typedef struct {
	double r;
	double i;
} complex_t;

uint32 find_poly_roots(double *poly, uint32 degree, complex_t *roots);

#ifdef __cplusplus
}
#endif

#endif /* _POLY_H_ */
