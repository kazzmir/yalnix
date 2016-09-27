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

#if MAX_POLY_DEGREE > 6
#error "polynomial degree > 6 not supported"
#endif

typedef struct {
	uint32 degree;
	double coeff[MAX_POLY_DEGREE + 1];
} dpoly_t;

typedef struct {
	double power;
	dpoly_t rpoly;
	dpoly_t apoly;
} dparam_t;

/*------------------------------------------------------------------*/
static double get_polyval(dpoly_t *poly, double x, double h) {

	/* evaluate poly(x+h) using a taylor series centered
	   at poly(x). We compute poly(x) and the correction
	   from poly(x) separately, because in practice x and h
	   can have such different magnitudes that x+h could
	   equal x to machine precision. */

	double base; 
	double off;
	double hpow;
	double *p = poly->coeff;

	switch (poly->degree) {
	case 0:
		base = p[0];
		off = 0;
		break;
	case 1:
		base = p[1]*x+p[0];
		off = p[1]*h;
		break;
	case 2:
		base = (p[2]*x+p[1])*x+p[0];
		hpow = h;
		off = hpow * (2*p[2]*x+p[1]);
		hpow *= h;
		off += hpow * p[2];
		break;
	case 3:
		base = ((p[3]*x+p[2])*x+p[1])*x+p[0];
		hpow = h;
		off = hpow * ((3*p[3]*x+2*p[2])*x+p[1]);
		hpow *= h;
		off += hpow * (3*p[3]*x+p[2]);
		hpow *= h;
		off += hpow * p[3];
		break;
	case 4:
		base = (((p[4]*x+p[3])*x+p[2])*x+p[1])*x+p[0];
		hpow = h;
		off = hpow * (((4*p[4]*x+3*p[3])*x+2*p[2])*x+p[1]);
		hpow *= h;
		off += hpow * ((6*p[4]*x+3*p[3])*x+p[2]);
		hpow *= h;
		off += hpow * (4*p[4]*x+p[3]);
		hpow *= h;
		off += hpow * p[4];
		break;
	case 5:
		base = ((((p[5]*x+p[4])*x+p[3])*x+p[2])*x+p[1])*x+p[0];
		hpow = h;
		off = hpow * ((((5*p[5]*x+4*p[4])*x+3*p[3])*x+2*p[2])*x+p[1]);
		hpow *= h;
		off += hpow * (((10*p[5]*x+6*p[4])*x+3*p[3])*x+p[2]);
		hpow *= h;
		off += hpow * ((10*p[5]*x+4*p[4])*x+p[3]);
		hpow *= h;
		off += hpow * (5*p[5]*x+p[4]);
		hpow *= h;
		off += hpow * p[5];
		break;
	case 6:
		base = (((((p[6]*x+p[5])*x+p[4])*x+p[3])*x+p[2])*x+p[1])*x+p[0];
		hpow = h;
		off = hpow * (((((6*p[6]*x+5*p[5])*x+4*p[4])*x+
						3*p[3])*x+2*p[2])*x+p[1]);
		hpow *= h;
		off += hpow * ((((15*p[6]*x+10*p[5])*x+
						6*p[4])*x+3*p[3])*x+p[2]);
		hpow *= h;
		off += hpow * (((20*p[6]*x+10*p[5])*x+4*p[4])*x+p[3]);
		hpow *= h;
		off += hpow * ((15*p[6]*x+5*p[5])*x+p[4]);
		hpow *= h;
		off += hpow * (6*p[6]*x+p[5]);
		hpow *= h;
		off += hpow * p[6];
		break;
	default:
		base = off = 0;
		break;
	}
	return base + off;
}

/*------------------------------------------------------------------*/
static double integrand(double x, double h, void *params) {

	/* callback for numerical integration */

	dparam_t *aux = (dparam_t *)params;
	double polyval;

	polyval = get_polyval(&aux->rpoly, x, h) *
		  get_polyval(&aux->apoly, x, h);
	return pow(fabs(polyval), aux->power);
}

/*------------------------------------------------------------------*/
static int compare_complex(const void *x, const void *y) {
	complex_t *xx = (complex_t *)x;
	complex_t *yy = (complex_t *)y;

	if (xx->r > yy->r)
		return 1;
	if (xx->r < yy->r)
		return -1;
	return 0;
}

/*------------------------------------------------------------------*/
#define INTEGRATE_LIMIT 1e12

static uint32 analyze_poly_size(poly_config_t *config,
				mp_poly_t *rpoly, mp_poly_t *apoly, 
				double *result) {

	uint32 i;
	uint32 rdeg, adeg;
	dparam_t params;
	complex_t roots[2*MAX_POLY_DEGREE];
	uint32 num_roots;
	double endpoints[4*MAX_POLY_DEGREE];
	uint32 num_endpoints;
	double accum;
	double err;
	double curr_endpoint;

	/* convert the polynomial coefficients from arbitrary
	   precision to double precision floating point */

	rdeg = params.rpoly.degree = rpoly->degree;
	for (i = 0; i <= rdeg; i++)
		params.rpoly.coeff[i] = mp_signed_mp2d(rpoly->coeff + i);

	adeg = params.apoly.degree = apoly->degree;
	for (i = 0; i <= adeg; i++)
		params.apoly.coeff[i] = mp_signed_mp2d(apoly->coeff + i);

	params.power = -2.0 / (rpoly->degree + apoly->degree);

	/* Rather than use Murphy's method to rate the average
	   size of polynomial values, we use a result of D.J.
	   Bernstein: For rational poly R(x) and algebraic poly
	   A(x), the number of polynomial values where x is rational
	   and the size of A(x)*R(x) achieves a given value is 
	   proportional to the integral of

	   	dx / ((R(x) * A(x))^2) ^ (1/(deg(R(x))+deg(A(x))))

	   for x from -infinity to +infinity. Larger values of
	   this integral imply more values that get found by
	   a siever. Bernstein writes that this estimate is 
	   "extremely accurate", but has not elaborated to date.
	   The integration variable x refers to the a/b values used
	   by a siever, and it is both more realistic and simpler to
	   make the integration interval finite but large 
	   
	   The integrand takes on a wide range of values depending
	   on how close x is to a root of R(x)*A(x), so we have to
	   partition the integral */


	/* find the roots of R(x)*A(x) and sort in order of 
	   increasing real part */

	if (find_poly_roots(params.rpoly.coeff, rdeg, roots))
		return 1;
	if (find_poly_roots(params.apoly.coeff, adeg, roots + rdeg))
		return 1;
	qsort(roots, (size_t)(rdeg + adeg),
			sizeof(complex_t), compare_complex);

	/* squeeze out roots whose real part would not lie
	   in the integration interval, along with most complex
	   roots and all complex conjugates of complex roots.
	   We cannot remove all complex roots because if the
	   imaginary part of the root is small then the value
	   of A(x)*R(x) is 'almost' a singularity at this point,
	   and ignoring it would throw off the numerical integrator */
	   
	for (i = num_roots = 0; i < rdeg + adeg; i++) {
		if (roots[i].i < 0.0 || 
		    roots[i].r <= -INTEGRATE_LIMIT ||
		    roots[i].r >= INTEGRATE_LIMIT)
			continue;
		if (roots[i].i < 5.0) 
			roots[num_roots++] = roots[i];
	}

	/* convert the collection of roots into a collection
	   of integration endpoints */

	num_endpoints = 0;
	curr_endpoint = endpoints[num_endpoints++] = -INTEGRATE_LIMIT;
	for (i = 0; i <= num_roots; i++) {
		double new_endpoint;

		if (i == num_roots)
			new_endpoint = INTEGRATE_LIMIT;
		else
			new_endpoint = roots[i].r;

		/* if the integration interval is very large, add
		   intermediate intervals. Since the integrand varies 
		   widely in size over the interval, without these 
		   intermediate endpoints to separate regions of 
		   similar integrand size the integrator will do 
		   2-4x as much work as it has to (it's not adaptive) */

		if (new_endpoint - curr_endpoint > 1e8) {
			if (fabs(new_endpoint) < fabs(curr_endpoint)) {
				endpoints[num_endpoints++] = new_endpoint-1e6;
				endpoints[num_endpoints++] = new_endpoint-1e3;
			}
			else {
				endpoints[num_endpoints++] = curr_endpoint+1e3;
				endpoints[num_endpoints++] = curr_endpoint+1e6;
			}
		}
		endpoints[num_endpoints++] = new_endpoint;
		curr_endpoint = new_endpoint;
	}

	/* proceed from interval to interval, adding up the
	   values of the integral and saving the maximum error */

	curr_endpoint = endpoints[0];
	accum = err = 0.0;

	for (i = 1; i < num_endpoints; i++) {
		double new_endpoint = endpoints[i];

		integrate_run(&config->integ_aux, integrand,
				&params, curr_endpoint, new_endpoint);

		accum += config->integ_aux.result;
		err = MAX(err, config->integ_aux.error);
		curr_endpoint = new_endpoint;
	}

	*result = accum;
	/* test for convergence */
	if (err > 1e-6 * fabs(accum))
		return 2;
	return 0;
}

/*------------------------------------------------------------------*/
#define NUM_SMALL_PRIMES 27

static double analyze_poly_roots(poly_config_t *config, mp_poly_t *poly) {

	/* analyze a polynomial for root properties (i.e.
	   compute Murphy's 'alpha' value). For the smallest primes
	   this uses the divisibility properties of a moderately
	   large set of polynomial sample values, and for larger 
	   primes we just count the roots that would appear in a 
	   factor base */

	uint32 i, j;
	double root_score;
	uint32 small_counts[NUM_SMALL_PRIMES];

	/* all linear polynomials have the same root properties */

	if (poly->degree == 1)
		return 0.0;

	/* experimentally determine how often the smallest primes
	   occur in a random sample of (a,b) pairs */

	memset(small_counts, 0, sizeof(small_counts));

	for (i = 0; i < NUM_POLY_SAMPLES; i++) {
		abpair_t *sample = config->samples + i;
		signed_mp_t sample_val;

		eval_poly(&sample_val, sample->a, sample->b, poly);

		for (j = 0; j < NUM_SMALL_PRIMES; j++) {
			uint32 prime = config->prime_list.list[j];
			uint32 num_factors = 0;
			while (mp_mod_1(&sample_val.num, prime) == 0) {
				num_factors++;
				mp_divrem_1(&sample_val.num, prime,
						&sample_val.num);
			}
			small_counts[j] += num_factors;
		}
	}

	/* compute the average contribution of the small primes */

	root_score = 0;
	for (i = 0; i < NUM_SMALL_PRIMES; i++) {
		uint32 prime = config->prime_list.list[i];
		root_score += log((double)prime) * 
				(1.0 / (prime - 1) - 
				 (double)small_counts[i] / NUM_POLY_SAMPLES);
	}

	/* for the larger primes, use Murphy's estimate for
	   the average contribution */

	for (; i < config->prime_list.num_primes; i++) {
		uint32 prime = config->prime_list.list[i];
		uint32 num_roots;
		uint32 dummy_roots[MAX_POLY_DEGREE];
		uint32 high_coeff;

		num_roots = poly_get_zeros(dummy_roots, poly, 
					prime, &high_coeff, 1);
		if (high_coeff == 0)
			num_roots++;

		root_score += (1.0 - (double)num_roots * prime / 
					(prime + 1)) * log((double)prime) / 
					(prime - 1);
	}

	return root_score;
}

/*------------------------------------------------------------------*/
static void print_poly(mp_poly_t *apoly) {

	uint32 i;
	char buf[1000];
	for (i = 0; i <= apoly->degree; i++) {
		signed_mp_t *c = apoly->coeff + i;
		printf("%c%s\n", (c->sign == NEGATIVE) ? '-' : ' ',
				mp_sprintf(&c->num, 10, buf));
	}
}

/*------------------------------------------------------------------*/
void analyze_poly(poly_config_t *config,
		      mp_poly_t *rpoly, mp_poly_t *apoly) {

	/* analyze a polynomial for sieving goodness.
	   If the polynomial is good enough, save it.
	  
	   The analysis routines are general enough so that
	   any polynomials can be tested, independent of
	   degree and skewness. The score assigned is
	   directly comparable to that of any other polynomials 
	   given to this routine. */

	double size_score;
	double root_score;
	double combined_score;

	if (analyze_poly_size(config, rpoly, apoly, &size_score))
		return;

	root_score = analyze_poly_roots(config, rpoly) +
	             analyze_poly_roots(config, apoly);

	/* An interesting problem: you have a root score and
	   a size score for each polynomial; how do you combine
	   them to get an overall rating? The value of root_score
	   implies that values of A(x) effectively have size
	   A(x) * exp(root_score). The second term is constant, so
	   pulling this expression out of the integrand used to 
	   calculate the size above gives: */

	combined_score = size_score * pow(exp(root_score), -2.0 /
					(rpoly->degree + apoly->degree));

	/* save the polynomial if its combined score is the 
	   best seen so far. A really thorough implementation
	   would do this in two stages: the best X polynomials
	   would all be kept in stage 1, and in stage 2 the 
	   complete list (possibly combined from the output of
	   different machines) would get a more accurate 
	   root_score computed for each polynomial. The top
	   few such polynomials would then be subjected to 
	   sieving experiments */

	if (combined_score > config->heap[0]->combined_score) {
		poly_select_t *entry = config->heap[0];

		entry->size_score = size_score;
		entry->root_score = root_score;
		entry->combined_score = combined_score;
		memcpy(&entry->rpoly, rpoly, sizeof(mp_poly_t));
		memcpy(&entry->apoly, apoly, sizeof(mp_poly_t));

		print_poly(rpoly);
		print_poly(apoly);
		printf("%le %lf %le\n", size_score, root_score, combined_score);
	}
}

/*------------------------------------------------------------------*/
void analyze_one_poly(msieve_obj *obj,
		      mp_poly_t *rpoly, mp_poly_t *apoly) {

	uint32 i;
	poly_config_t config;

	poly_config_init(obj, &config);
	analyze_poly(&config, rpoly, apoly);

	for (i = 0; i <= rpoly->degree; i++) {
		logprintf(obj, "R%u: %c%s\n", i,
			(rpoly->coeff[i].sign == POSITIVE? ' ' : '-'),
			mp_sprintf(&rpoly->coeff[i].num, 10,
						obj->mp_sprintf_buf));
	}
	for (i = 0; i <= apoly->degree; i++) {
		logprintf(obj, "A%u: %c%s\n", i,
			(apoly->coeff[i].sign == POSITIVE? ' ' : '-'),
			mp_sprintf(&apoly->coeff[i].num, 10,
						obj->mp_sprintf_buf));
	}
	logprintf(obj, "size score = %e, Murphy alpha = %f, combined = %e\n",
			config.heap[0]->size_score,
			config.heap[0]->root_score,
			config.heap[0]->combined_score);
	poly_config_free(&config);
}

/*------------------------------------------------------------------*/
void poly_config_init(msieve_obj *obj, poly_config_t *config) {

	/* one-time initialization for polynomial search */

	uint32 i, num_primes;

	fill_prime_list(&config->prime_list, 0xffff, PRIME_BOUND);
	num_primes = config->prime_list.num_primes;

	config->heap_num_filled = 0;
	for (i = 0; i < POLY_HEAP_SIZE; i++) {
		config->heap[i] = (poly_select_t *)calloc((size_t)1, 
					sizeof(poly_select_t));
		if ( ! config->heap[i] ){
			TtyPrintf( 0, "no free memory\n" );
			exit( -1 );
		}
	}

	integrate_init(&config->integ_aux, 1e-100, 1e-8, 
			double_exponential, 8);

	config->samples = (abpair_t *)calloc((size_t)NUM_POLY_SAMPLES,
						sizeof(abpair_t));
	if ( ! config->samples ){
		TtyPrintf( 0, "no free memory\n" );
		exit( -1 );
	}

	/* when analyzing root properties, all polynomials
	   are sampled at the same set of points */

	i = 0;
	while (i < NUM_POLY_SAMPLES) {
		int32 a = -10000000 +
			(int32)get_rand(&obj->seed1, &obj->seed2) % 20000000;
		uint32 b = get_rand(&obj->seed1, &obj->seed2) % 500000;

		if (b > 0 && mp_gcd_1((uint32)abs(a), b) == 1) {
			config->samples[i].a = a;
			config->samples[i].b = b;
			i++;
		}
	}
}

/*------------------------------------------------------------------*/
void poly_config_free(poly_config_t *config) {

	uint32 i;

	free(config->prime_list.list);
	for (i = 0; i < POLY_HEAP_SIZE; i++)
		free(config->heap[i]);
	free(config->samples);
	integrate_free(&config->integ_aux);
}
