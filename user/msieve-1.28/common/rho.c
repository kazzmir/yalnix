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

#include <common.h>

/* simplified implementation of Pollard Rho factoring
   (more specifially, Rho with the improvements due to Brent).
   The tuning assumes finding 6-8 digit factors */

/* perform this many iterations between GCDs */
#define BATCH_SIZE 64

uint32 rho(msieve_obj *obj, mp_t *n, 
		factor_list_t *factor_list) {

	uint32 i;
	uint32 max_fail; 
	uint32 num_fail = 0;
	uint32 num_rounds; 
	mp_t tmp_n;
	uint32 factor_found = 0;
	uint32 bits = mp_bits(n);

	mp_copy(n, &tmp_n);

	/* We will keep trying until max_fail pseudorandom 
	   functions fail to find a factor */

	if (bits < 150)
		max_fail = 2;
	else if (bits < 250)
		max_fail = 3;
	else
		max_fail = 4;

	/* compute the number of iterations to use for
	   each pseudorandom function. This is a multiple
	   of the batch size so that GCD's only have to
	   occur in one place. We do not reduce this
	   bound if a factor is found; factors found
	   are likely to be the smallest in n, and we
	   don't want to hurt our chances of finding larger
	   factors by reducing the amount of work done */

	if (bits < 100)
		num_rounds = 50 * BATCH_SIZE;
	else if (bits < 200)
		num_rounds = 100 * BATCH_SIZE;
	else
		num_rounds = 250 * BATCH_SIZE;


	/* for each function */

	while (num_fail < max_fail) {

		/* the function is x[n] = x[n-1]^2 + inc mod n,
		   with inc and x[0] chosen randomly. We can be
		   much more careful about the choice of inc and
		   x[0], and in particular Pari chooses inc from
		   a carefully constructed set of values that
		   ensure no duplicate iterations for x[0] = 2.
		   In practice I doubt that it makes much difference */

		mp_t x, y, acc, t;
		uint32 inc = get_rand(&obj->seed1, &obj->seed2);
		uint32 limit = 2;
		uint32 parity = 0;

		mp_clear(&x);
		x.nwords = 1;
		x.val[0] = get_rand(&obj->seed1, &obj->seed2);
		mp_copy(&x, &y);
		mp_clear(&acc);
		acc.nwords = acc.val[0] = 1;

		/* for each iteration */

		for (i = 1; i <= num_rounds; i++) {

			/* compute the next function value. Do not
			   reduce mod n after adding inc, since it
			   is so rare for x to exceed n that it's
			   faster to just try another function when
			   x gets corrupted */

			mp_modmul(&x, &x, &tmp_n, &x);
			mp_add_1(&x, inc, &x);

			/* multiply the product of gcd inputs by
			   (y - new_x). Remember the sign of the
			   product to avoid having to add n */

			if (mp_cmp(&y, &x) >= 0) {
				mp_sub(&y, &x, &t);
			}
			else {
				mp_sub(&x, &y, &t);
				parity ^= 1;
			}
			mp_modmul(&acc, &t, &tmp_n, &acc);

			/* possibly compute a GCD, and quit if
			   the result exceeds 1 */

			if (i % BATCH_SIZE == 0) {
				if (parity == 1)
					mp_sub(&tmp_n, &acc, &acc);
				mp_gcd(&acc, &tmp_n, &t);
				if (!mp_is_one(&t))
					break;
				mp_copy(&t, &acc);
				parity = 0;
			}

			/* possibly update y */

			if (i + 1 == limit) {
				mp_copy(&x, &y);
				limit *= 2;
			}
		}

		/* t contains the last GCD value computed. If
		   this is not n, and we haven't run out of iterations,
		   then a nontrivial factor has been found */

		if (i <= num_rounds && mp_cmp(&t, &tmp_n) != 0) {
			mp_t q, r;

			/* remove all instances of the factor from n */

			while (1) {
				mp_divrem(&tmp_n, &t, &q, &r);
				if (mp_is_zero(&q) || !mp_is_zero(&r))
					break;
				mp_copy(&q, &tmp_n);
			}
			factor_found = 1;

			/* give up if the new n is small enough for
			   other methods to be more efficient */

			if (factor_list_add(obj, factor_list, &t) < 80)
				break;
		}
		else {
			num_fail++;
		}
	}

	return factor_found;
}
