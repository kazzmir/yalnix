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

#ifndef _INTEGRATE_H_
#define _INTEGRATE_H_

#include "util.h"

#ifdef __cplusplus
extern "C" {
#endif

/* generic interface for performing numerical integration */

/* type of integrator */

enum integrator_type {
	double_exponential
};

/* callback to evaluate the integrand at one point (base+offset).
   The two components of the point are provided separately because
   they could have widely varying magnitudes */

typedef double (*integrand_t)(double base, double offset, void *params);

/* structure controlling one integration */

typedef struct {
	double result;		/* value of last integration */
	double error;		/* absolute error in last integration */

	enum integrator_type type;/* type of integrator */
	void *internal;		/* integrator-specific data */
} integrate_t;

/* set up for one integration. min_val is the smallest
   value such that will ever be encountered when calculating
   any future integrals, relative_error is the desired error 
   tolerance for all future integrations using this struct, 
   and num_levels controls how hard the integrator will work */

void integrate_init(integrate_t *aux, double min_val,
			double relative_error,
			enum integrator_type type,
			uint32 num_levels);

/* clean up integration struct */

void integrate_free(integrate_t *aux);

/* compute the integral of func from lower_limit to upper_limit. */

uint32 integrate_run(integrate_t *aux, integrand_t func, void *params,
			double lower_limit, double upper_limit);

#ifdef __cplusplus
}
#endif

#endif /* !_INTEGRATE_H_ */
