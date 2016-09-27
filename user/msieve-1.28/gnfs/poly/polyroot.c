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

/* Polynomial rootfinder using Laguerre's algorithm. The
   implementation is loosely based on the one from 'Numerical
   Recipes in C' */

/*---------------------------------------------------------------------*/
static INLINE complex_t cplx_set(double re, double im) {
	complex_t c;
	c.r = re;
	c.i = im;
	return c;
}

/*---------------------------------------------------------------------*/
static INLINE complex_t cplx_add(complex_t a, complex_t b) {
	return cplx_set(a.r + b.r, a.i + b.i);
}

/*---------------------------------------------------------------------*/
static INLINE complex_t cplx_sub(complex_t a, complex_t b) {
	return cplx_set(a.r - b.r, a.i - b.i);
}

/*---------------------------------------------------------------------*/
static INLINE complex_t cplx_mul(complex_t a, complex_t b) {
	return cplx_set(a.r * b.r - a.i * b.i,
			a.r * b.i + a.i * b.r);
}

/*---------------------------------------------------------------------*/
static INLINE complex_t cplx_mul_scalar(complex_t a, double b) {
	return cplx_set(b * a.r, b * a.i);
}

/*---------------------------------------------------------------------*/
static INLINE double cplx_norm(complex_t a) {
	double xr = fabs(a.r);
	double xi = fabs(a.i);
	if (xr == 0.0) {
		return xi;
	}
	else if (xi == 0.0) {
		return xr;
	}
	else if (xr > xi) {
		double q = xi / xr;
		return xr * sqrt(1.0 + q * q);
	}
	else {
		double q = xr / xi;
		return xi * sqrt(1.0 + q * q);
	}
}

/*---------------------------------------------------------------------*/
static INLINE complex_t cplx_div(complex_t a, complex_t b) {

	if (fabs(b.r) >= fabs(b.i)) {
		double q = b.i / b.r;
		double den = b.r + q * b.i;
		return cplx_set((a.r + q * a.i) / den,
				(a.i - q * a.r) / den);
	}
	else {
		double q = b.r / b.i;
		double den = b.i + q * b.r;
		return cplx_set((a.r * q + a.i) / den,
				(a.i * q - a.r) / den);
	}
}

/*---------------------------------------------------------------------*/
static INLINE complex_t cplx_sqrt(complex_t a) {

	double xr, xi, w, r;

	if (a.r == 0.0 && a.i == 0.0)
		return a;
	
	xr = fabs(a.r);
	xi = fabs(a.i);
	if (xr >= xi) {
		r = xi / xr;
		w = sqrt(xr) * sqrt(0.5 * (1.0 + sqrt(1.0 + r * r)));
	}
	else {
		r = xr / xi;
		w = sqrt(xi) * sqrt(0.5 * (r + sqrt(1.0 + r * r)));
	}

	if (a.r >= 0.0) {
		return cplx_set(w, a.i / (2.0 * w));
	}
	else {
		complex_t c;
		c.i = (a.i >= 0.0) ? w : -w;
		c.r = a.i / (2.0 * c.i);
		return c;
	}
}

/*---------------------------------------------------------------------*/
#define MAX_ITER 80

static uint32 find_one_root(complex_t *poly, uint32 degree,
				complex_t x, complex_t *root, double eps) {
	uint32 i, j;
	complex_t g, g2, gp, gm, h, sq, dx;
	double abp, abm;
	double m = (double)degree;

	for (i = 0; i < MAX_ITER; i++) {
		complex_t b = poly[degree];
		complex_t d = cplx_set(0.0, 0.0);
		complex_t f = cplx_set(0.0, 0.0);
		double err = cplx_norm(b);
		double abx = cplx_norm(x);

		for (j = degree - 1; (int32)j >= 0; j--) {
			f = cplx_add(cplx_mul(x, f), d);
			d = cplx_add(cplx_mul(x, d), b);
			b = cplx_add(cplx_mul(x, b), poly[j]);
			err = cplx_norm(b) + abx * err;
		}

		err = err * eps;
		if (cplx_norm(b) <= err) {
			*root = x;
			return 0;
		}

		g = cplx_div(d, b);
		g2 = cplx_mul(g, g);
		h = cplx_sub(g2, cplx_mul_scalar(cplx_div(f, b), 2.0));
		sq = cplx_sub(cplx_mul_scalar(h, m), g2);
		sq = cplx_sqrt(cplx_mul_scalar(sq, m - 1));

		gp = cplx_add(g, sq);
		abp = cplx_norm(gp);
		gm = cplx_sub(g, sq);
		abm = cplx_norm(gm);
		if (abp < abm) {
			gp = gm;
			abp = abm;
		}

		if (abp == 0.0)
			return 1;

		dx = cplx_div(cplx_set(m, 0.0), gp);
		x = cplx_sub(x, dx);
	}

	return 2;
}

/*---------------------------------------------------------------------*/
static void deflate_poly(complex_t *poly, uint32 degree,
			complex_t root) {
	uint32 i;
	complex_t tmp_coeff = poly[degree];

	for (i = degree - 1; (int32)i >= 0; i--) {
		complex_t t = poly[i];
		poly[i] = tmp_coeff;
		tmp_coeff = cplx_add(cplx_mul(root, tmp_coeff), t);
	}
}

/*---------------------------------------------------------------------*/
uint32 find_poly_roots(double *poly, uint32 degree, complex_t *roots) {

	uint32 i, j;
	complex_t start_coeff[MAX_POLY_DEGREE + 1];
	complex_t coeff[MAX_POLY_DEGREE + 1];
	double start;

	if (degree == 0) {
		return 0;
	}
	else if (degree == 1) {
		roots[0] = cplx_set(-poly[0] / poly[1], 0.0);
		return 0;
	}

	/* convert from double to complex */

	for (i = 0; i <= degree; i++)
		start_coeff[i] = coeff[i] = cplx_set(poly[i], 0.0);
	
	/* for each root */

	i = degree;
	j = 0;
	start = 0.0;
	while (i) {
		complex_t next_root; 
		complex_t polished_root; 
		
		/* start the iteration from an initial point near
		   the origin, to increase the odds of finding the root
		   of coeff(x) that has the smallest magnitude. We
		   may have to try multiple starting guesses, since 
		   the derivatives of poly must not be zero at the
		   starting point */

		while (find_one_root(coeff, i, cplx_set(start, 0.0), 
					&next_root, 1e-14) == 1) {
			start += 0.05;
		}

		/* change roots with very small imaginary part to
		   be explicitly real roots */

		if (fabs(next_root.i) < 1e-14 * fabs(next_root.r))
			next_root.i = 0.0;

		deflate_poly(coeff, i--, next_root);

		/* polish the root. With an initial tolerance close
		   to that of the machine epsilon, this step is only
		   necessary if there was some kind of numerical 
		   instability deflating the polynomial, and we expect
		   this to be very unlikely */

		if (find_one_root(start_coeff, degree,
				next_root, &polished_root, 1e-14))
			return 1;

		/* save the polished root, along with its complex
		   comnjugate if the root is complex */

		roots[j++] = polished_root;
		if (next_root.i != 0.0) {
			next_root.i = -next_root.i;
			deflate_poly(coeff, i--, next_root);

			roots[j].r = polished_root.r;
			roots[j].i = -polished_root.i;
			j++;
		}
	}

	return 0;
}
