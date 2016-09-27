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

#ifndef _COMMON_H_
#define _COMMON_H_

#include <msieve.h>

#ifdef __cplusplus
extern "C" {
#endif

/*--------------PRIME SIEVE RELATED DECLARATIONS ---------------------*/

typedef struct {
	uint32 p;
	uint32 r;
} prime_aux_t;

typedef struct {
	uint32 num_aux;
	prime_aux_t *aux;
	uint8 *sieve;
	uint32 curr_block;
	uint32 curr_off;
} prime_sieve_t;

void init_prime_sieve(prime_sieve_t *sieve, 
			uint32 min_prime,
			uint32 max_prime);

void free_prime_sieve(prime_sieve_t *sieve);

uint32 get_next_prime(prime_sieve_t *sieve);

typedef struct {
	uint32 *list;
	uint32 num_primes;
} prime_list_t;

void fill_prime_list(prime_list_t *prime_list, 
			uint32 max_size, 
			uint32 max_prime);

/*--------------DECLARATIONS FOR MANAGING FACTORS FOUND -----------------*/

typedef struct {
	mp_t factor;
	enum msieve_factor_type type;
} final_factor_t;

typedef struct {
	uint32 num_factors;
	final_factor_t *final_factors[256];
} factor_list_t;

void factor_list_init(factor_list_t *list);

void factor_list_free(mp_t *n, factor_list_t *list, msieve_obj *obj);

uint32 factor_list_add(msieve_obj *obj, 
			factor_list_t *list, 
			mp_t *new_factor);

/*--------------DECLARATIONS FOR FACTORING METHODS -----------------*/

/* perform trial factoring on an integer.
   Returns 1 if any factors were found, 0 if not */

#define TRIAL_FACTOR_BOUND 100000
#define TRIAL_FACTOR_NUM_PRIMES 9590

uint32 trial_factor(msieve_obj *obj, mp_t *n, mp_t *reduced_n,
		    prime_list_t *prime_list,
		    factor_list_t *factor_list);

/* Attempt to factor an integer using Pollard's Rho method.
   Returns 1 if any factors were found, 0 if not */

uint32 rho(msieve_obj *obj, mp_t *n, factor_list_t *factor_list);

/* Factor a number up to 62 bits in size using the SQUFOF
   algorithm. Returns zero if the factorization failed for 
   whatever reason, otherwise returns one factor up to 31 bits.
   Note that the factor returned may be 1, indicating a
   trivial factorization you probably don't want */

uint32 squfof(mp_t *n);

/* Factor a number up to 85 bits in size using MPQS. 
   Returns 0 on failure and nonzero on success, with
   factor1 and factor2 filled in on success. NOT TO
   BE USED for high-throughput applications */

uint32 tinyqs(mp_t *n, mp_t *factor1, mp_t *factor2);

/* Factor a number using the full MPQS implementation. 
   Returns 1 if any factors were found and 0 if not */

uint32 factor_mpqs(msieve_obj *obj, mp_t *n, factor_list_t *factor_list);

/* Factor a number using GNFS. Returns
   1 if any factors were found and 0 if not */

uint32 factor_gnfs(msieve_obj *obj, mp_t *n, factor_list_t *factor_list);

#define MIN_NFS_BITS 320

/*--------------LINEAR ALGEBRA RELATED DECLARATIONS ---------------------*/

/* Used to represent a list of relations */

typedef struct {
	uint32 num_relations;  /* number of relations in the cycle */
	uint32 *list;          /* list of offsets into an array of relations */
} la_cycle_t;

/* A column of the matrix */

typedef struct {
	uint32 *data;		/* The list of occupied rows in this column */
	uint32 weight;		/* Number of nonzero entries in this column */
	la_cycle_t cycle;       /* list of relations comprising this column */
} la_col_t;

/* merge src1[] and src2[] into merge_array[], assumed
   large enough to hold the merged result. Return the
   final number of elements in merge_array */

uint32 merge_relations(uint32 *merge_array,
		  uint32 *src1, uint32 n1,
		  uint32 *src2, uint32 n2);

uint64 * block_lanczos(msieve_obj *obj,
			uint32 nrows, 
			uint32 num_dense_rows,
			uint32 ncols, 
			la_col_t *cols,
			uint32 *deps_found);

void count_matrix_nonzero(msieve_obj *obj,
			uint32 nrows, uint32 num_dense_rows,
			uint32 ncols, la_col_t *cols);

void reduce_matrix(msieve_obj *obj, uint32 *nrows, 
		uint32 num_dense_rows, uint32 *ncols, 
		la_col_t *cols, uint32 num_excess);

void free_cycle_list(la_col_t *cycle_list, uint32 num_cycles);

/*-------------- MISCELLANEOUS STUFF ----------------------------------*/

#define POSITIVE 0
#define NEGATIVE 1

/* For big factorizations, some people have reported errors
   that indicate that the savefiles were corrupted. I have
   sometimes noticed (in other programs) that high-speed 
   formatted output to a text file will occaisionally get 
   corrupted.

   Hence, all writes to the savefile are manually buffered,
   so that disk writes are always in large blocks that get
   flushed immediately. */

#define SAVEFILE_BUF_SIZE 65536

/* emit logging information */

void logprintf(msieve_obj *obj, char *fmt, ...);

/* convert an expression into an mp_t; returns 0 on
   success, negative value on failure */

int32 evaluate_expression(char *expr, mp_t *res);

/* remember a factor that was just found */

void add_next_factor(msieve_obj *obj, mp_t *n, 
		enum msieve_factor_type factor_type);

#ifdef __cplusplus
}
#endif

#endif /* _COMMON_H_ */
