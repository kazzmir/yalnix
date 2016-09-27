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

#include <integrate.h>

/* This code implements numerical integration via double-exponential (DE)
   transformation. Basically the integrand is evaluated at a set of
   points that approach the limits of the integration interval faster
   than exponentially, with weights that tend to zero faster than
   exponentially. The result is that even if the integrand has singularities
   at both endpoints, as long as it is analytic and doesn't vary by 
   much in the interior the interval, this method converges to the numerical
   value of the integral with comparatively very few function evaluations.
   Basically we use the trapezoid rule in transformed coordinates and
   reuse previous computations when cutting the stepsize in half. The
   code computes the integral value, the residual, and an error estimate
   at every step. The DE transformation is comparatively unknown (I used
   to be very interested in numerical integration and had never heard of
   it before Brian Gladman brought it to my attention), and is applicable
   to a wide variety of different integration-type problems. For further 
   explanations, history and a bunch of references see M. Mori, 'The 
   Discovery of the Double Exponential Transformation its Developments'.

   The code here is based on code from 1996 by Takuya Ooura, which was a
   C port from the original painfully obscure spaghetti fortran. I've cleaned 
   it up a lot and documented how I think things work, but it's always 
   possible I misinterpreted what the original code was doing */


/* bound on the amount of memory allocated */

#define MAX_TRAP_LEVELS 8

/* macro for evaluating the integrand. base is always
   the left or right integration endpoint, and the integrand
   is evaluated at (base+off). The two must be provided separately,
   because base and off can be of such different magnitudes that
   adding them together could cause numerical instability. This
   happens when base+off == base to the limit of double precision.
   When the endpoints are singularities it is important to calculate
   the contribution of off separately */

#define FUNC_EVAL(base, off) func(base, off, params)

/*---------- double exponential integrator, definite interval ------------*/

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif

/* describes one point in the interval to evaluate */

typedef struct {
	double abscissa;        /* distance from the left or right endpoint
				   where the integrand is evaluated */
	double weight;          /* weight applied to the integrand at
				   'abscissa' before adding to integral value */
	double residual_weight; /* weight applied to the integrand at
				   'abscissa' before adding to residual */
} de_point_t;

/* structure controlling numerical integration. Note that introductory
   numerical methods describe the trapezoid rule as having equal-size
   panels, with the rule at half the step size containing twice as many
   half-size panels. The DE transform is different: we compute the
   integral from -1 to 1 via a transformation to an integral from -infinity
   to +infinity, then evaluate the latter integral using a series of
   trapezoid panels that increase super-exponentially in width. The 'step-size'
   refers to the distance of the first sample point from the origin, and
   halving the step-size here means doubling the number of trapezoid panels. 
   While there are really an infinite number of trapezoid panels to sum, 
   we truncate the sum when the size of the abscissas become too large to 
   be represented accurately. Because the abscissas increase in size so 
   quickly, it only takes a few samples to reach this point.  The de_point_t 
   structures store the value of the abscissa after conversion back to the 
   (-1,1) interval, as offsets from +-1 */

typedef struct {
	uint32 num_trap_levels;    /* number of step size halvings allowed */
	uint32 num_points;         /* maximum number of trapezoid panels
				      allowed, beyond which the abscissas
				      are of size close to the limit of double 
				      precision */
	uint32 initial_num_points; /* maximum number of trapezoid panels to
				      use such that the transformed integrand
				      is assumed to have no trouble with 
				      roundoff or truncation error */
	double relative_error;     /* the overall allowed relative error */
	double truncate_scale;     /* multiplier of the initial error that
				      is used to bound the minimum size of 
				      function values, beyond which no con-
				      tribution to the answer is expected */
	double abscissa_size_cutoff; /* multiplier of the initial error used
					to determine when the integrator has
					converged to the integral value */
	de_point_t center_point;   /* initial sample point of the interval */
	de_point_t *initial_rule;  /* further trapezoid sample points at the 
				      coarsest step size (1.0). There are 
				      num_points of these, and since the rule 
				      is symmetric about the origin we have
				      up to 2*num_points samples at this
				      resolution */
	de_point_t *refined_rules; /* list of groups of num_points samples
				      that implement the trapezoid rule with
				      successively halved step sizes. For
				      level i, 0 <= i < num_trap_levels, the
				      step size (i.e. the distance of the
				      first abscissa to the origin) is
				      2^-(i+1) and there are 2^i groups of
				      up to num_points samples to add. Sample
				      groups are stored in order of increasing 
				      i, then increasing value of initial
				      abscissa. Each group of samples is also
				      symmetric about the origin.  */
} de_t;

/*-----------------------------------------------------------------------*/
static void de_fill_sample_group(de_point_t *rule, 
			uint32 num_points, double first_sample, 
			double abscissa_range,
			double expo_large, double expo_small) {

	/* fill the information for the num_points trapezoid
	   panels, with the first abscissa at (first_sample * 
	   abscissa_range), 0 < first_sample <= 1. The sample 
	   points march off to infinity in transformed coordinates;
	   they are stored in untransformed coordinates, as offsets 
	   from +-abscissa_range, converging super-exponentially 
	   quickly to zero */

	uint32 i;
	double curr_expo = exp(first_sample * abscissa_range);
	double curr_expo_large = M_PI_2 * curr_expo;
	double curr_expo_small = M_PI_2 / curr_expo;

	for (i = 0; i < num_points; i++) {
		de_point_t *curr_point = rule + i;
		double abscissa = 1.0 / (1.0 + exp(curr_expo_large -
						   curr_expo_small));
		double weight = abscissa * (1.0 - abscissa) * abscissa_range;

		curr_point->abscissa = abscissa;
		curr_point->weight = weight * (curr_expo_large +
						curr_expo_small);
		curr_point->residual_weight = 4.0 * weight;

		curr_expo_large *= expo_large;
		curr_expo_small *= expo_small;
	}
}

/*-----------------------------------------------------------------------*/
static void de_init(integrate_t *aux, double min_val,
			double relative_error,
			uint32 num_levels_in) {

	/* initialize for all future intergations using aux */

	/* parameters of the algorithm: 
	   
	   relative_error_scale is the amount of extra paranoia 
	   added to the bound on the error that the code will try 
	   to satisfy; it's needed because we calculate the error 
	   for one progression of sample values and there are 
	   several such progressions, each of which is assumed to 
	   incur its own error

	   abscissa_scale expands the DE region from (-1,1) to 
	   (-abscissa_scale,abscissa_scale), presumably to avoid 
	   truncation error */

	const double relative_error_scale = 0.1;
	const double abscissa_scale = 8.5;

	uint32 i, j;
	de_t *integ;
	de_point_t *curr_samples;
	uint32 num_points;
	uint32 num_levels;
	double log_min_val = -log(min_val);
	double log_relative_error = 1.0 - log(relative_error_scale *
						relative_error);
	double abscissa_range = abscissa_scale / log_relative_error;
	double expo_large = exp(abscissa_range);
	double expo_small = 1.0 / expo_large;

	/* fill error bounds */

	integ = (de_t *)malloc(sizeof(de_t));
	integ->relative_error = relative_error;
	integ->truncate_scale = exp(-expo_small * log_relative_error);
	integ->abscissa_size_cutoff = sqrt(relative_error_scale * 
						relative_error);

	/* describe the initial sample point, in the middle 
	   of the interval */

	integ->center_point.weight = M_PI_2 * abscissa_range * 0.5;
	integ->center_point.residual_weight = abscissa_range;

	/* figure out the maximum number of samples needed
	   and the number of step-size halvings allowed */

	num_levels = integ->num_trap_levels = MIN(num_levels_in, 
						MAX_TRAP_LEVELS);
	num_points = integ->num_points = (uint32)(log(log_min_val / M_PI_2) /
						 abscissa_range) + 1;


	curr_samples = integ->initial_rule = (de_point_t *)malloc(
					(num_points << num_levels) *
					sizeof(de_point_t));

	/* create the initial set of trapezoid sample points, with
	   step size of 1.0 * abscissa_range */

	de_fill_sample_group(curr_samples, num_points, 1.0,
				abscissa_range, expo_large, expo_small);

	/* compute the number of samples to use before looking
	   out for truncation error. This value applies to all
	   groups of samples used by the integrator */

	for (i = 0; i < num_points; i++) {
		if (curr_samples[i].abscissa < integ->abscissa_size_cutoff)
			break;
	}
	integ->initial_num_points = MIN(i + 1, num_points);
	curr_samples += num_points;
	integ->refined_rules = curr_samples;

	/* now create trapezoid sample points in order of increasing
	   level. Level i has 2^i groups of samples; the j_th group
	   contains samples that start a distance of 
	   abscissa_range*j*2^-(i+1) from the origin and march away 
	   to inifinity in transformed coordinates at a given rate
	   (identical for all groups). Combining all the samples 
	   at level i, along with initial_rule and all samples at 
	   levels < i, produces the complete set of samples needed 
	   to implement the trapezoid rule with step size 2^-(i+1) */

	for (i = 0; i < num_levels; i++) {
		uint32 num_groups = 1 << i;
		double step_size = 1.0 / num_groups;
		double sample_val = 0.5 * step_size;

		for (j = 0; j < num_groups; j++) {
			de_fill_sample_group(curr_samples, num_points, 
					sample_val, abscissa_range, 
					expo_large, expo_small);
			sample_val += step_size;
			curr_samples += num_points;
		}
	}

	aux->internal = (void *)integ;
}

/*-----------------------------------------------------------------------*/
static void de_free(integrate_t *aux) {

	de_t *i = (de_t *)(aux->internal);
	free(i->initial_rule);
	free(i);
	aux->internal = NULL;
}

/*-----------------------------------------------------------------------*/
static uint32 de_run(integrate_t *aux, integrand_t func, void *params,
			double lower_limit, double upper_limit) {

	/* compute the integral of func(x, params) from lower_limit 
	   <= x <= upper_limit */

	uint32 i, j, k;
	de_t *integ = (de_t *)aux->internal;
	de_point_t *points;
	uint32 num_points = integ->num_points;
	uint32 initial_num_points = integ->initial_num_points;
	uint32 final_num_points;
	uint32 num_levels = integ->num_trap_levels;
	double interval = upper_limit - lower_limit;
	double step_size = 1.0;
	uint32 num_groups = 1;

	double result;
	double residual;
	double error;
	double truncate_error;
	double target_error;
	double curr_error;

	double left_val = 0.0;
	double right_val = 0.0;

	/* evaluate func at the middle of the interval */

	result = FUNC_EVAL(0.5 * (lower_limit + upper_limit), 0.0);
	residual = result * integ->center_point.residual_weight;
	result = result * integ->center_point.weight;
	error = fabs(result);
	points = integ->initial_rule;

	/* compute the trapezoid rule approximation at the coarsest
	   step size. The error is assumed to be the sum of the
	   magnitudes of all the samples. Sample at abscissa values
	   that are symmetric about the origin */

	for (i = 0; i < initial_num_points; i++) {
		de_point_t *p = points + i;
		double abscissa = interval * p->abscissa;

		left_val = FUNC_EVAL(lower_limit, abscissa);
		right_val = FUNC_EVAL(upper_limit, -abscissa);
		residual += (left_val + right_val) * p->residual_weight;
		left_val *= p->weight;
		right_val *= p->weight;
		result += left_val + right_val;
		error += fabs(left_val) + fabs(right_val);
	}

	/* determine the smallest function value that will 
	   contribute to the integral or the residual */

	truncate_error = error * integ->truncate_scale;

	/* add samples at the coarsest resolution until the
	   sample values become too small to matter. Handle
	   the left and right endpoints of the integration
	   interval separately, since each may need a different
	   number of samples */

	for (j = initial_num_points; j < num_points; j++) {
		de_point_t *p = points + j;
		double abscissa = interval * p->abscissa;

		if (fabs(left_val) <= truncate_error)
			break;

		left_val = FUNC_EVAL(lower_limit, abscissa);
		residual += left_val * p->residual_weight;
		left_val *= p->weight;
		result += left_val;
	}
	final_num_points = j - 1;

	for (j = initial_num_points; j < num_points; j++) {
		de_point_t *p = points + j;
		double abscissa = interval * p->abscissa;

		if (fabs(right_val) <= truncate_error)
			break;

		right_val = FUNC_EVAL(upper_limit, -abscissa);
		residual += right_val * p->residual_weight;
		right_val *= p->weight;
		result += right_val;
	}
	final_num_points = MIN(final_num_points, j - 1);

	/* now compute trapezoid rules corresponding to successively
	   halved step sizes, until the estimate of the error indicates
	   convergence to the desired error tolerance */

	target_error = error * integ->abscissa_size_cutoff;
	curr_error = 1.0 + 2.0 * target_error;
	points = integ->refined_rules;

	for (i = 0; i < num_levels && 
			curr_error > target_error; i++) {

		double old_result = result;
		double old_residual = residual;

		/* compute the trapezoid rule with step size 2^-(i+1) by
		   adding in 2^i groups of additional sample points.
		   Each sample falls somewhere in between two previous
	           samples, and all groups march from near the middle
		   of the interval out to the integration endpoints, all
		   in a similar manner (we assume the integrand behaves
		   the same for all groups as it did at the coarsest
		   step size) */

		for (j = 0; j < num_groups; j++, points += num_points) {

			/* update the integral value and residual */

			for (k = 0; k < final_num_points; k++) {
				de_point_t *p = points + k;
				double abscissa = interval * p->abscissa;

				left_val = FUNC_EVAL(lower_limit, abscissa);
				right_val = FUNC_EVAL(upper_limit, -abscissa);
				residual += (left_val + right_val) * 
							p->residual_weight;
				result += (left_val + right_val) * p->weight;
			}

			/* repeat until the sample values become in-
			   consequentially small */

			for (k = final_num_points; k < num_points; k++) {
				de_point_t *p = points + k;
				double abscissa = interval * p->abscissa;

				left_val = FUNC_EVAL(lower_limit, abscissa);
				residual += left_val * p->residual_weight;
				left_val *= p->weight;
				result += left_val;

				if (fabs(left_val) <= truncate_error)
					break;
			}

			for (k = final_num_points; k < num_points; k++) {
				de_point_t *p = points + k;
				double abscissa = interval * p->abscissa;

				right_val = FUNC_EVAL(upper_limit, -abscissa);
				residual += right_val * p->residual_weight;
				right_val *= p->weight;
				result += right_val;

				if (fabs(right_val) <= truncate_error)
					break;
			}
		}

		/* use the computed integral value and residual, at
		   the two different step sizes, to update the error 
		   estimate */

		curr_error = step_size * (fabs(result - 2.0 * old_result) +
					  fabs(residual - 2.0 * old_residual));
		step_size *= 0.5;
		num_groups *= 2;
	}

	/* compute the final integral value */

	aux->result = result * interval * step_size;

	/* if the integration converged, ignore the error
	   estimate and say that it converged; otherwise use
	   the error estimate directly. We assume that the
	   maximum error occurs for every group of samples, 
	   and since each group is symmetric there are effectively
	   twice as many groups (computed above) */

	if (curr_error > target_error) {
		aux->error = curr_error * num_groups * interval;
		return 1;
	}
	else {
		aux->error = error * integ->relative_error *
					num_groups * interval;
		return 0;
	}
}

/*------------------------- external interface --------------------------*/

void integrate_init(integrate_t *aux, double min_val,
			double relative_error,
			enum integrator_type type,
			uint32 num_levels) {

	aux->type = type;
	if (type == double_exponential)
		de_init(aux, min_val, relative_error, num_levels);
}

void integrate_free(integrate_t *aux) {

	if (aux->type == double_exponential)
		de_free(aux);
}

uint32 integrate_run(integrate_t *aux, integrand_t func, void *params,
			double lower_limit, double upper_limit) {

	aux->result = 0.0;
	aux->error = 0.0;
	if (aux->type == double_exponential)
		return de_run(aux, func, params, lower_limit, upper_limit);
	return 1;
}
