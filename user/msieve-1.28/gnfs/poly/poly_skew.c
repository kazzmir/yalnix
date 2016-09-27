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

#include "poly.h"
#include "skew_sieve.h"

#if MAX_POLY_DEGREE < 6
#error "Polynomial generation assumes degree <= 6 allowed"
#endif

#define MAX_RAT_FACTORS 200
#define MAX_AUX_RAT_FACTORS 400

#define MAX_CURR_RAT_FACTORS 10

/* main structure for representing the factors of
   the leading rational poly coefficient */

typedef struct {
	uint16 prime;
	uint16 num_roots : 3;
	uint16 id : 13;
	uint16 roots[MAX_POLY_DEGREE];
} rat_factor_t;

static char buf[1024];

/*------------------------------------------------------------------*/
static void print_poly(mp_poly_t *apoly) {

	uint32 i;
	for (i = 0; i <= apoly->degree; i++) {
		signed_mp_t *c = apoly->coeff + i;
		printf("%c%s\n", (c->sign == NEGATIVE) ? '-' : ' ',
				mp_sprintf(&c->num, 10, buf));
	}
}

/*------------------------------------------------------------------*/
static void create_poly(mp_t *n, uint32 degree,
		      mp_t *r1, mp_t *r0, 
		      uint64 high_coeff,
		      mp_poly_t *rpoly,
		      mp_poly_t *apoly) {

	uint32 i;
	signed_mp_t r0_pow[MAX_POLY_DEGREE + 1];
	signed_mp_t inv_r0_pow[MAX_POLY_DEGREE];
	signed_mp_t curr_n;
	signed_mp_t t;
	signed_mp_t *coeff;
	mp_t r0_half;
	
	memset(apoly, 0, sizeof(mp_poly_t));
	apoly->degree = degree;

	mp_copy(r0, &r0_pow[1].num);
	r0_pow[1].sign = POSITIVE;
	for (i = 2; i <= degree; i++) {
		signed_mp_mul(r0_pow + i - 1, r0_pow + 1, r0_pow + i);
	}

	if (mp_modinv(r0, r1, &inv_r0_pow[1].num) != 0) {
		printf("no modular inverse\n");
		exit(-1);
	}
	inv_r0_pow[1].sign = POSITIVE;
	for (i = 2; i < degree; i++) {
		mp_modmul(&inv_r0_pow[i-1].num, &inv_r0_pow[1].num, 
				r1, &inv_r0_pow[i].num);
		inv_r0_pow[i].sign = POSITIVE;
	}

	coeff = apoly->coeff + degree;
	signed_mp_clear(coeff);
	coeff->num.nwords = 1;
	coeff->num.val[0] = (uint32)high_coeff;
	if (high_coeff >> 32) {
		coeff->num.val[coeff->num.nwords++] = 
				(uint32)(high_coeff >> 32);
	}

	mp_copy(n, &curr_n.num);
	curr_n.sign = POSITIVE;

	for (i = degree - 1, coeff--; (int32)i >= 0; i--, coeff--) {

		signed_mp_mul(coeff + 1, r0_pow + i + 1, &t);
		signed_mp_sub(&curr_n, &t, &t);
		mp_div(&t.num, r1, &curr_n.num);
		curr_n.sign = t.sign;

		if (i > 0) {
			mp_t rem;

			mp_div(&curr_n.num, &r0_pow[i].num, &coeff->num);
			coeff->sign = curr_n.sign;

			signed_mp_mul(&curr_n, inv_r0_pow + i, &t);

			signed_mp_sub(&t, coeff, &t);
			mp_mod(&t.num, r1, &rem);
			if (t.sign == NEGATIVE && !mp_is_zero(&rem))
				mp_sub(r1, &rem, &rem);

			mp_copy(&rem, &t.num);
			t.sign = POSITIVE;
			signed_mp_add(coeff, &t, coeff);
		}
	}

	signed_mp_copy(&curr_n, coeff + 1);

	/* change the coefficients to be less than r0 / 2
	   in absolute value */

	mp_rshift(r0, 1, &r0_half);
	mp_copy(r1, &t.num);
	t.sign = POSITIVE;

	for (i = 0; i < degree; i++) {
		signed_mp_t *c0 = apoly->coeff + i;
		signed_mp_t *c1 = c0 + 1;
		if (mp_cmp(&c0->num, &r0_half) > 0) {
			if (c0->sign == NEGATIVE) {
				signed_mp_add(c0, r0_pow + 1, c0);
				signed_mp_sub(c1, &t, c1);
			}
			else {
				signed_mp_sub(c0, r0_pow + 1, c0);
				signed_mp_add(c1, &t, c1);
			}
		}
	}

	/* form the rational polynomial */

	memset(rpoly, 0, sizeof(mp_poly_t));
	rpoly->degree = 1;
	mp_copy(r0, &rpoly->coeff[0].num);
	rpoly->coeff[0].sign = NEGATIVE;
	mp_copy(r1, &rpoly->coeff[1].num);
	rpoly->coeff[1].sign = POSITIVE;
}

/*------------------------------------------------------------------*/
static void poly_skew_stage3(mp_t *n, poly_config_t *config, 
				uint32 degree, mp_t *r1, mp_t *r0, 
				uint64 high_coeff, double max_skew) {

	mp_poly_t rpoly;
	mp_poly_t apoly;

	create_poly(n, degree, r1, r0, high_coeff, &rpoly, &apoly);

	printf("%c%s\n", (apoly.coeff[degree-1].sign == NEGATIVE) ? '-' : ' ',
			mp_sprintf(&apoly.coeff[degree-1].num, 10, buf));

	if (fabs(mp_mp2d(&apoly.coeff[degree - 2].num)) > max_skew)
		return;

	analyze_poly(config, &rpoly, &apoly);
	print_poly(&rpoly);
	print_poly(&apoly);
	printf("\n");
}

/*------------------------------------------------------------------*/
static void poly_skew_stage2(mp_t *n, uint32 degree,
			poly_config_t *config, 
			rat_factor_t *rat_factors, 
			uint32 num_rat_factors, 
			mp_t *m0_in, uint64 alg_coeff,
			double max_skew) {

	uint32 i, j;
	mp_t r1, r0;
	mp_t m0, t;
	mp_t crt_tmp[MAX_CURR_RAT_FACTORS];
	mp_t accum[MAX_CURR_RAT_FACTORS + 1];
	uint32 i0, i1, i2, i3, i4, i5, i6, i7, i8, i9;

	/* compute r1, the high-order rational coefficient */

	mp_clear(&r1);
	r1.nwords = 1;
	r1.val[0] = rat_factors[0].prime;
	for (i = 1; i < num_rat_factors; i++)
		mp_mul_1(&r1, rat_factors[i].prime, &r1);

	printf("r1: %s\n", mp_sprintf(&r1, 10, buf));

	/* compute the auxiliary quantities needed for CRT
	   reconstruction of r0 */

	for (i = 0; i < num_rat_factors; i++) {

		uint32 p = rat_factors[i].prime;

		mp_divrem_1(&r1, p, crt_tmp + i);
		j = mp_mod_1(crt_tmp + i, p);
		j = mp_modinv_1(j, p);
		mp_mul_1(crt_tmp + i, j, crt_tmp + i);
	}
	mp_clear(accum + i);

	/* round m0 to the nearest smaller multiple of r1 */

	mp_mod(m0_in, &r1, &t);
	mp_sub(m0_in, &t, &m0);
	printf("m0: %s\n", mp_sprintf(&m0, 10, buf));

	i0 = i1 = i2 = i3 = i4 = i5 = i6 = i7 = i8 = i9 = 0;

	switch (num_rat_factors) {
	case 10:
		for (i9 = rat_factors[9].num_roots - 1; (int32)i9 >= 0; i9--) {
			mp_mul_1(crt_tmp + 9, rat_factors[9].roots[i9], &t);
			mp_add(accum + 10, &t, accum + 9);
	case 9:
		for (i8 = rat_factors[8].num_roots - 1; (int32)i8 >= 0; i8--) {
			mp_mul_1(crt_tmp + 8, rat_factors[8].roots[i8], &t);
			mp_add(accum + 9, &t, accum + 8);
	case 8:
		for (i7 = rat_factors[7].num_roots - 1; (int32)i7 >= 0; i7--) {
			mp_mul_1(crt_tmp + 7, rat_factors[7].roots[i7], &t);
			mp_add(accum + 8, &t, accum + 7);
	case 7:
		for (i6 = rat_factors[6].num_roots - 1; (int32)i6 >= 0; i6--) {
			mp_mul_1(crt_tmp + 6, rat_factors[6].roots[i6], &t);
			mp_add(accum + 7, &t, accum + 6);
	case 6:
		for (i5 = rat_factors[5].num_roots - 1; (int32)i5 >= 0; i5--) {
			mp_mul_1(crt_tmp + 5, rat_factors[5].roots[i5], &t);
			mp_add(accum + 6, &t, accum + 5);
	case 5:
		for (i4 = rat_factors[4].num_roots - 1; (int32)i4 >= 0; i4--) {
			mp_mul_1(crt_tmp + 4, rat_factors[4].roots[i4], &t);
			mp_add(accum + 5, &t, accum + 4);
	case 4:
		for (i3 = rat_factors[3].num_roots - 1; (int32)i3 >= 0; i3--) {
			mp_mul_1(crt_tmp + 3, rat_factors[3].roots[i3], &t);
			mp_add(accum + 4, &t, accum + 3);
	case 3:
		for (i2 = rat_factors[2].num_roots - 1; (int32)i2 >= 0; i2--) {
			mp_mul_1(crt_tmp + 2, rat_factors[2].roots[i2], &t);
			mp_add(accum + 3, &t, accum + 2);
	case 2:
		for (i1 = rat_factors[1].num_roots - 1; (int32)i1 >= 0; i1--) {
			mp_mul_1(crt_tmp + 1, rat_factors[1].roots[i1], &t);
			mp_add(accum + 2, &t, accum + 1);
	case 1:
		for (i0 = rat_factors[0].num_roots - 1; (int32)i0 >= 0; i0--) {
			mp_mul_1(crt_tmp + 0, rat_factors[0].roots[i0], &t);
			mp_add(accum + 1, &t, accum + 0);
			mp_mod(accum + 0, &r1, &t);
			mp_add(&m0, &t, &r0);
			printf("%u,%u,%u,%u %s\t", i3, i2, i1, i0,
					mp_sprintf(&t, 10, buf));
			poly_skew_stage3(n, config, degree, &r1, &r0, 
						alg_coeff, max_skew);
		}}}}}}}}}}
	}
	exit(-1);
}


/*------------------------------------------------------------------*/
static uint32 find_rat_roots(mp_t *n, uint32 degree,
				uint16 *factor_list,
				uint32 num_factors,
				rat_factor_t *rat_factors,
				uint32 max_rat_factors,
				uint64 alg_coeff) {

	uint32 i, j, k;

	for (i = j = 0; i < num_factors; i++) {
		uint32 p = factor_list[i];
		rat_factor_t *curr_factor = rat_factors + j;
		uint32 nmodp = mp_mod_1(n, p);
		uint32 amodp = alg_coeff % p;
		uint32 count = 0;

		if (amodp == 0 || mp_gcd_1(amodp, p) > 1) 
			continue;

		amodp = mp_modinv_1(amodp, p);
		nmodp = mp_modmul_1(nmodp, amodp, p);

		/* find modular roots of nmodp, if they exist */

		curr_factor->prime = p;
		for (k = 1; k < p; k++) {
			if (mp_expo_1(k, degree, p) == nmodp) {
				curr_factor->roots[count] = k;
				if (++count == degree)
					break;
			}
		}

		if (count > 0) {
			/* make curr_factor remember that it is
			   entry j in the list */
			curr_factor->id = j;
			curr_factor->num_roots = count;
			if (++j == max_rat_factors)
				break;
		}
	}

//	printf("%u factors\n", j);
	return j;
}

/*------------------------------------------------------------------*/
static void find_poly_stage1(mp_t *n, uint32 degree,
			poly_config_t *config, uint16 *factor_list, 
			uint32 num_rat_factors, rat_sieve_t *rat_sieve,
			uint64 alg_coeff) {

	/* after the leading algebraic poly coefficient is
	   chosen we have to choose the leading rational
	   coefficient. We do so in bulk here */

	uint32 i, j;
	uint32 max_rat_factors;
	uint32 max_aux_rat_factors;
	uint32 max_curr_rat_factors;
	rat_factor_t rat_factors[MAX_RAT_FACTORS];
	rat_factor_t aux_rat_factors[MAX_AUX_RAT_FACTORS];
	rat_factor_t curr_rat_factors[MAX_CURR_RAT_FACTORS];
	mp_t m0, tmp1, tmp2;
	double max_skew;
	double max_r1;
	double approx_r1;

	/* In order to generate valid NFS polynomial pairs, 
	   we have to choose the rational polynomial R(x) = 
	   R1 * x - R0. This routine picks R1 and 
	   find_poly_stage2() picks R0. If the leading 
	   algebraic coefficient is A we must have 
	   
	   	N == A*R0^degree mod R1   (*)
	   
	   R1 will be the product of several small primes; 
	   a prime p is suitable for use within R1 if (*) has
	   one or more solutions mod p. That way we can pick
	   several p and use the Chinese Remainder Thm. to solve
	   for R0. We want p to have many solutions to (*), so
	   we can generate many R0 for a given collection of 
	   primes p, to give more chances to find a good algebraic
	   polynomial. 
	   
	   For degree d a prime p has at most d solutions to (*).
	   The loop below creates a pool of primes p that do not
	   divide high_coeff and have at least 1 solution to (*). */

	max_rat_factors = find_rat_roots(n, degree, 
					factor_list, num_rat_factors,
					rat_factors, MAX_RAT_FACTORS,
					alg_coeff);

	max_aux_rat_factors = find_rat_roots(n, degree, 
					rat_sieve->factor_list,
					rat_sieve->num_factors,
					aux_rat_factors, MAX_AUX_RAT_FACTORS,
					alg_coeff);

	/* determine a bound on the number factors the leading 
	   rational coefficient will have. This depends on the size 
	   of the leading algebraic coefficient; we want the
	   product of the primes chosen to be larger, but
	   not too much larger, than the algebraic coefficient.
	   This is because the second highest algebraic coefficient
	   is bounded in size by the leading rational coefficient */

	if (alg_coeff < 1.0e4)
		max_curr_rat_factors = 5;
	else if (alg_coeff < 1.0e6)
		max_curr_rat_factors = 6;
	else if (alg_coeff < 1.0e8)
		max_curr_rat_factors = 7;
	else if (alg_coeff < 1.0e10)
		max_curr_rat_factors = 8;
	else
		max_curr_rat_factors = 9;

	/* compute m0 ( = (N/A)^(1/degree) ), the starting 
	   value for the rational coefficient R0. Each 
	   distinct R1 will modify m0 slightly to get a 
	   suitable R0. */

	mp_clear(&tmp1);
	tmp1.val[0] = (uint32)alg_coeff;
	tmp1.val[1] = (uint32)(alg_coeff >> 32);
	tmp1.nwords = 1;
	if (tmp1.val[1])
		tmp1.nwords = 2;

	mp_div(n, &tmp1, &tmp2);
	mp_iroot(&tmp2, degree, &m0);

	/* find the largest value of R1 that we will tolerate. This
	   in turn requires finding the approximate largest skew in
	   the polynomial coefficients. We assume that this largest
	   skew causes all polynomial coefficients to contribute
	   equally to the algebraic norm */

	if (degree == 4)
		max_skew = 3.8;
	else if (degree == 5)
		max_skew = 3.5;
	else
		max_skew = 3.0;

	max_skew = exp((mp_log(&m0) - log((double)alg_coeff)) / max_skew);
	max_r1 = 2.0 * max_skew * alg_coeff;
	printf("max skew %8.0lf, max_r1 %12.0lf, max_a%d %20.0lf %u\n", 
				max_skew, max_r1, degree - 2, 
				max_skew * max_skew * alg_coeff,
				(uint32)alg_coeff);

	max_skew = max_skew * max_skew * alg_coeff;

	/* now enumerate all the collections of i primes in the
	   pool, and generate the polynomials for each collection
	   whose combined product is smaller than the bound on R1.
	   The enumeration algorithm is a wonderful combinatorial 
	   method called NEXKSB by Nijenhuis and Wilf */

	for (i = max_curr_rat_factors - 2; i <= max_curr_rat_factors; i++) {

		uint32 first_entry = 0;
		uint32 list_size = i;

		do {
			/* assemble the primes that have changed from
			   the last iteration */

			for (j = 0; j < list_size; j++) {
				curr_rat_factors[i + j - list_size] =
						rat_factors[first_entry + j];
			}

			/* if the current r1 exceeds the r1 bound then 
			   fast-forward to the next i-tuple that has a 
			   smaller prime in it */

			approx_r1 = curr_rat_factors[0].prime;
			for (j = 1; j < i; j++)
				approx_r1 *= curr_rat_factors[j].prime;

			if (approx_r1 > max_r1) {
				if (++list_size > i) /* future r1 all fail */
					break;

				first_entry = curr_rat_factors[i - 
							list_size].id + 1;
				continue;
			}

			/* generate NFS polynomials */

			poly_skew_stage2(n, degree, config, 
					curr_rat_factors, i, &m0,
					alg_coeff, max_skew);

			for (j = 0; j < max_aux_rat_factors; j++) {
				if (approx_r1 * aux_rat_factors[j].prime > 
							max_r1) {
					break;
				}

				curr_rat_factors[i] = aux_rat_factors[j];
				poly_skew_stage2(n, degree, config, 
						curr_rat_factors, i + 1, &m0,
						alg_coeff, max_skew);
			}

			/* figure out how many entries have to change
			   in the next iteration. Most of the time only
			   one entry changes */

			if (first_entry < max_rat_factors - list_size)
				list_size = 0;
			list_size++;

			/* figure out the ordinal number of the entries
			   they will change into */

			first_entry = curr_rat_factors[i - list_size].id + 1;

			/* enumeration stops when the last i-tuple of
			   primes is loaded */
		} while (curr_rat_factors[0].id != max_rat_factors - i);
	}
}

/*------------------------------------------------------------------*/
static void find_poly_core(msieve_obj *obj, mp_t *n,
			poly_config_t *config,
			uint32 degree, uint32 deadline) {

	uint32 i;
	uint32 default_cofactor;
	alg_sieve_t alg_sieve;
	rat_sieve_t rat_sieve;
	uint64 alg_coeff_min = 1;
	uint32 coeffs_done;
	time_t start_time, curr_time;
	uint16 rat_factors[MAX_RAT_FACTORS];
	uint32 num_rat_factors;

	/* all leading algebraic coefficients will be 
	   divisible by default_cofactor. This is chosen 
	   both to ease the sieving burden and to give 
	   all polynomials more projective roots for the 
	   small primes */

	default_cofactor = 2 * 2 * 3 * 5;

	alg_sieve_init(alg_coeff_min, &alg_sieve);

	/* the leading rational poly coefficient will 
	   be the product of several small primes p > 5.
	   Separate out the p that = 1 mod degree, as
	   these will build many related algebraic polynomials */

	for (i = 3, num_rat_factors = 0; 
				i < config->prime_list.num_primes; i++) {

		uint32 p = config->prime_list.list[i];

		if (p > MAX_RAT_FACTOR_SIZE)
			break;

		if (p % degree == 1) {
			rat_factors[num_rat_factors] = p;
			if (++num_rat_factors == MAX_RAT_FACTORS)
				break;
		}
	}

	rat_sieve_init(&rat_sieve, rat_factors, num_rat_factors);

	coeffs_done = 0;
	time(&start_time);
	while (1) {

		uint64 next_coeff = default_cofactor * 
					alg_sieve_next(&alg_sieve);

		find_poly_stage1(n, degree, config, rat_factors, 
				num_rat_factors, &rat_sieve, next_coeff);

		/* quit if interrupted */

		if (obj->flags & MSIEVE_FLAG_STOP_SIEVING)
			break;

		/* print progress message, quit if time expired */

		if (++coeffs_done % 512 == 0) {
			break;
			time(&curr_time);
			fprintf(stderr, "%u%% done (processed %u blocks)\r", 
				MIN(100, (uint32)(100.0 * difftime(
				    curr_time, start_time) / deadline + 0.5)),
				coeffs_done);
			fflush(stderr);
			if (difftime(curr_time, start_time) > deadline)
				break;
		}
	}

	fprintf(stderr, "\n");
	alg_sieve_free(&alg_sieve);
	rat_sieve_free(&rat_sieve);
}

/*------------------------------------------------------------------*/
void find_poly_skew(msieve_obj *obj, mp_t *n,
			poly_config_t *config,
			uint32 deadline) {

	/* search for NFS polynomials */

	uint32 bits = mp_bits(n);

	if (bits < 331) {		/* <= 100 digits */
		find_poly_core(obj, n, config, 4, deadline);
	}
	else if (bits < 347) {		/* 100-105 digits */
		find_poly_core(obj, n, config, 4, deadline / 2);
		find_poly_core(obj, n, config, 5, deadline / 2);
	}
	else if (bits < 592) {		/* 105-180 digits */
		find_poly_core(obj, n, config, 5, deadline);
	}
	else if (bits < 659) {		/* 180-200 digits */
		find_poly_core(obj, n, config, 5, deadline / 2);
		find_poly_core(obj, n, config, 6, deadline / 2);
	}
	else {				/* 200+ digits */
		find_poly_core(obj, n, config, 6, deadline);
	}
}
