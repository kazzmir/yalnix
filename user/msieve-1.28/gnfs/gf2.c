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
#include "gnfs.h"

/* the number of quadratic characters for each
   matrix column. A practical upper limit given
   in Buhler et. al. is 3 * log2(n), while 
   Bernstein writes that a QCB size of 50 'should 
   be enough for any possible NFS factorization'. 
   If each quadratic character reduces by half the
   odds that the linear algebra will not produce
   an algebraic square, then a very few characters
   (say 16) will be enough. The matrix-building code 
   is flexible enough so that this number may take 
   any positive value and everything else will 
   just work */

#define QCB_SIZE 32

/*------------------------------------------------------------------*/
static int compare_uint32(const void *x, const void *y) {
	uint32 *xx = (uint32 *)x;
	uint32 *yy = (uint32 *)y;
	if (*xx > *yy)
		return 1;
	if (*xx < *yy)
		return -1;
	return 0;
}

/*------------------------------------------------------------------*/
static int compare_ideals(const void *x, const void *y) {
	
	/* used to determine the ordering of two ideals.
	   Ordering is by prime, then by root of prime,
	   then by rational or algebraic type. The ordering 
	   by prime is tricky, because -1 has a special value 
	   that must be explicitly accounted for. This ordering
	   is designed to put the most dense matrix rows first */

	ideal_t *k = (ideal_t *)x;
	ideal_t *t = (ideal_t *)y;

	if (k->i.compressed_p == 0x7fffffff ||
	    t->i.compressed_p == 0x7fffffff) {
		if (k->i.compressed_p == 0x7fffffff &&
		    t->i.compressed_p != 0x7fffffff)
			return -1;
		else if (k->i.compressed_p != 0x7fffffff &&
		         t->i.compressed_p == 0x7fffffff)
			return 1;
		return 0;
	}

	if (k->i.compressed_p < t->i.compressed_p)
		return -1;
	if (k->i.compressed_p > t->i.compressed_p)
		return 1;
		
	if (k->i.r < t->i.r)
		return -1;
	if (k->i.r > t->i.r)
		return 1;

	if (k->i.rat_or_alg < t->i.rat_or_alg)
		return -1;
	if (k->i.rat_or_alg > t->i.rat_or_alg)
		return 1;
		
	return 0;
}

/*------------------------------------------------------------------*/
#define MAX_COL_IDEALS 1000

static uint32 combine_relations(la_col_t *col, relation_t *rlist,
				ideal_t *merged_ideals,
				fb_entry_t *qcb, uint32 *dense_rows,
				uint32 num_dense_rows) {

	uint32 i, j, k;
	uint32 num_merged = 0;
	ideal_t tmp_ideals[MAX_COL_IDEALS];
	uint32 num_tmp_ideals;

	/* form the matrix column corresponding to a 
	   collection of relations */

	for (i = 0; i < col->cycle.num_relations; i++) {
		relation_t *r = rlist + col->cycle.list[i];
		relation_lp_t new_ideals;
		int64 a = r->a;
		uint32 b = r->b;

		/* compute the quadratic characters for r, place
		   in the first few dense rows */

		for (j = 0; j < QCB_SIZE; j++) {
			uint32 p = qcb[j].p;
			uint32 r = qcb[j].r;
			int64 res = a % (int64)p;
			int32 symbol;

			if (res < 0)
				res += (int64)p;

			symbol = mp_legendre_1(mp_modsub_1((uint32)res,
					mp_modmul_1(b, r, p), p), p);

			/* symbol must be 1 or -1; if it's 0,
			   there's something wrong with the choice
			   of primes in the QCB but this isn't
			   a fatal error */

			if (symbol == -1)
				dense_rows[j / 32] ^= 1 << (j % 32);
			else if (symbol == 0)
				printf("warning: zero character\n");
		}

		/* if r is a free relation, modify the last dense row */

		if (b == 0) {
			dense_rows[(num_dense_rows - 1) / 32] ^=
					1 << ((num_dense_rows - 1) % 32);
		}

		/* get the ideal decomposition of relation i, sort
		   by size of prime */

		if (find_large_ideals(r, &new_ideals, 0, 0) > 
						TEMP_FACTOR_LIST_SIZE) {
			printf("error: overflow reading ideals\n");
			exit(-1);
		}
		if (num_merged + new_ideals.ideal_count >= MAX_COL_IDEALS) {
			printf("error: overflow merging ideals\n");
			exit(-1);
		}
		if (new_ideals.ideal_count > 1) {
			qsort(new_ideals.ideal_list, 
					(size_t)new_ideals.ideal_count,
					sizeof(ideal_t), compare_ideals);
		}

		/* merge it with the current list of ideals */

		j = k = 0;
		num_tmp_ideals = 0;
		while (j < num_merged && k < new_ideals.ideal_count) {
			int32 compare_result = compare_ideals(
						merged_ideals + j,
						new_ideals.ideal_list + k);
			if (compare_result < 0) {
				tmp_ideals[num_tmp_ideals++] = 
						merged_ideals[j++];
			}
			else if (compare_result > 0) {
				tmp_ideals[num_tmp_ideals++] = 
						new_ideals.ideal_list[k++];
			}
			else {
				j++; k++;
			}
		}
		while (j < num_merged) {
			tmp_ideals[num_tmp_ideals++] = merged_ideals[j++];
		}
		while (k < new_ideals.ideal_count) {
			tmp_ideals[num_tmp_ideals++] = 
						new_ideals.ideal_list[k++];
		}

		num_merged = num_tmp_ideals;
		memcpy(merged_ideals, tmp_ideals, 
				num_merged * sizeof(ideal_t));

		/* within the cycle structure, change from a pointer 
		   to relation i back to the relation number for i */

		col->cycle.list[i] = r->rel_index;
	}

	return num_merged;
}

/*------------------------------------------------------------------*/
static ideal_t *fill_small_ideals(factor_base_t *fb,
					uint32 *num_ideals_out) {
	uint32 i, j;
	uint32 last_p;
	uint32 num_ideals;
	ideal_t *small_ideals;
	fb_entry_t *entries;

	if (MAX_PACKED_PRIME == 0) {
		*num_ideals_out = 0;
		return NULL;
	}

	/* count the number of rational ideals (include -1).
	   Ideals to the same prime are counted only once */

	entries = fb->rfb.entries;
	for (i = last_p = 0, num_ideals = 1; 
			entries[i].p <= MAX_PACKED_PRIME; i++) {

		uint32 p = entries[i].p;
		if (p != last_p) {
			last_p = p;
			num_ideals++;
		}
	}

	/* add in the number of algebraic ideals. Note that if
	   a prime appears in the list, later code requires that
	   *all* of its roots appear there too */

	entries = fb->afb.entries;
	for (i = 0; entries[i].p <= MAX_PACKED_PRIME; i++)
		num_ideals++;

	small_ideals = (ideal_t *)malloc(num_ideals * sizeof(ideal_t));
	if ( ! small_ideals ){
		TtyPrintf( 0, "no free memory\n" );
		exit( -1 );
	}

	/* fill in the rational ideal of -1 */

	small_ideals[0].i.rat_or_alg = RATIONAL_IDEAL;
	small_ideals[0].i.compressed_p = 0x7fffffff;
	small_ideals[0].i.r = (uint32)(-1);

	/* fill in the other small rational ideals */

	entries = fb->rfb.entries;
	for (i = 1, j = last_p = 0; entries[j].p <= MAX_PACKED_PRIME; j++) {

		uint32 p = entries[j].p;

		if (p != last_p) {
			small_ideals[i].i.rat_or_alg = RATIONAL_IDEAL;
			small_ideals[i].i.compressed_p = (p - 1) / 2;
			small_ideals[i].i.r = p;
			last_p = p;
			i++;
		}
	}

	/* repeat for the algebraic ideals */

	entries = fb->afb.entries;
	for (j = 0; entries[j].p <= MAX_PACKED_PRIME; i++, j++) {
		small_ideals[i].i.rat_or_alg = ALGEBRAIC_IDEAL;
		small_ideals[i].i.compressed_p = (entries[j].p - 1) / 2;
		small_ideals[i].i.r = entries[j].r;
	}

	/* put the ideals in order of increasing size */

	qsort(small_ideals, (size_t)num_ideals, 
			sizeof(ideal_t), compare_ideals);

	*num_ideals_out = num_ideals;
	return small_ideals;
}

/*------------------------------------------------------------------*/
static void fill_qcb(fb_entry_t *qcb, factor_base_t *fb,
			uint32 min_qcb_ideal) {
	uint32 i, j;
	mp_poly_t *poly = &fb->afb.poly;
	prime_sieve_t sieve;

	/* construct the quadratic character base, starting
	   with the primes above min_qcb_ideal */

	init_prime_sieve(&sieve, min_qcb_ideal, 0xffffffff);

	i = 0;
	while (i < QCB_SIZE) {
		uint32 roots[MAX_POLY_DEGREE];
		uint32 num_roots, high_coeff;
		uint32 p = get_next_prime(&sieve);

		num_roots = poly_get_zeros(roots, poly, p, 
					&high_coeff, 0);

		/* p cannot be a projective root of the
		   algebraic poly */

		if (high_coeff == 0)
			continue;

		/* save the next batch of roots */

		for (j = 0; i < QCB_SIZE && j < num_roots; i++, j++) {
			qcb[i].p = p;
			qcb[i].r = roots[j];
		}
	}

	free_prime_sieve(&sieve);
}

/*------------------------------------------------------------------*/
#define LOG2_IDEAL_HASHTABLE_SIZE 20

static void build_matrix(msieve_obj *obj, factor_base_t *fb,
			uint32 *nrows, uint32 *dense_rows_out, 
			uint32 *ncols, la_col_t **cols_out) {

	/* read in the relations that contribute to the
	   initial matrix, and form the quadratic characters
	   for each column */

	uint32 i, j, k;
	uint32 num_relations;
	uint32 num_cycles;
	la_col_t *cycle_list;
	relation_t *rlist;
	hashtable_t unique_ideals;
	uint32 min_qcb_ideal;
	fb_entry_t qcb[QCB_SIZE];
	ideal_t *small_ideals;
	uint32 num_small_ideals;
	uint32 *dense_rows;
	uint32 num_dense_rows;
	uint32 dense_row_words;
	FILE *matrix_fp;
	char buf[256];

	sprintf(buf, "%s.mat", obj->savefile_name);
	matrix_fp = fopen(buf, "w+b");
	if (matrix_fp == NULL) {
		logprintf(obj, "error: can't open matrix file '%s'\n", buf);
		exit(-1);
	}

	/* fill in the small ideals */

	small_ideals = fill_small_ideals(fb, &num_small_ideals);

	/* we need extra matrix rows to make sure that each
	   dependency has an even number of relations, and also an
	   even number of free relations. If the rational 
	   poly R(x) is monic, and we weren't using free relations,
	   the sign of R(x) is negative for all relations, meaning we 
	   already get the effect of the extra rows. However, in 
	   general we can't assume both of these are true */
	
	num_dense_rows = QCB_SIZE + 2 + num_small_ideals;
	dense_row_words = (num_dense_rows + 31) / 32;
	dense_rows = (uint32 *)malloc(dense_row_words * sizeof(uint32));
	if ( ! dense_rows ){
		TtyPrintf( 0, "no free memory\n" );
		exit( -1 );
	}

	/* trim the factor base size */

	free(fb->rfb.entries);
	free(fb->afb.entries);
	fb->rfb.entries = NULL;
	fb->afb.entries = NULL;

	/* read in the cycles that form the matrix columns,
	   and the relations they will need */

	nfs_read_cycles(obj, fb, &num_cycles, &cycle_list, 
			&num_relations, &rlist, 1, 0);

	/* find the largest algebraic factor across the whole set */

	for (i = min_qcb_ideal = 0; i < num_relations; i++) {
		relation_t *r = rlist + i;
		uint32 *factors_a = r->factors + r->num_factors_r;
		for (j = 0; j < r->num_factors_a; j++)
			min_qcb_ideal = MAX(min_qcb_ideal, factors_a[j]);
	}

	/* choose the quadratic character base from the primes
	   larger than any that appear in the list of relations */

	logprintf(obj, "using %u quadratic characters above %u\n",
				QCB_SIZE, min_qcb_ideal + 1);

	fill_qcb(qcb, fb, min_qcb_ideal + 1);

	/* now form the actual matrix columns */

	hashtable_init(&unique_ideals, LOG2_IDEAL_HASHTABLE_SIZE,
			10000, (uint32)(sizeof(ideal_t)/sizeof(uint32)));

	/* for each cycle */

	for (i = 0; i < num_cycles; i++) {
		la_col_t *c = cycle_list + i;
		ideal_t merged_ideals[MAX_COL_IDEALS];
		uint32 mapped_ideals[MAX_COL_IDEALS];
		uint32 num_merged;

		/* dense rows start off empty */

		for (j = 0; j < dense_row_words; j++)
			dense_rows[j] = 0;

		/* merge the relations in the cycle and compute
		   the quadratic characters */

		num_merged = combine_relations(c, rlist, merged_ideals, qcb, 
						dense_rows, num_dense_rows);

		/* fill in the parity row, and place at 
		   bit position QCB_SIZE */

		if (c->cycle.num_relations % 2)
			dense_rows[QCB_SIZE / 32] |= 1 << (QCB_SIZE % 32);

		/* assign a unique number to each ideal in 
		   the cycle. This will automatically ignore
		   empty rows in the matrix */

		for (j = k = 0; j < num_merged; j++) {
			ideal_t *ideal = merged_ideals + j;
			uint32 p = ideal->i.compressed_p;

			if (MAX_PACKED_PRIME > 0 && (p == 0x7fffffff || 
			    p <= (MAX_PACKED_PRIME-1)/2) ) {
				/* dense ideal; store in compressed format */
				ideal_t *loc = (ideal_t *)bsearch(ideal, 
						small_ideals,
						(size_t)num_small_ideals,
						sizeof(ideal_t),
						compare_ideals);
				uint32 idx = QCB_SIZE + 1 +
						(loc - small_ideals);
				if (loc == NULL) {
					printf("error: unexpected dense ideal "
						"(%08x,%08x) found\n",
						ideal->blob[0], ideal->blob[1]);
					exit(-1);
				}
				dense_rows[idx / 32] |= 1 << (idx % 32);
			}
			else {
				hash_t *entry = hashtable_find(&unique_ideals, 
							ideal->blob, NULL);
				mapped_ideals[k++] = num_dense_rows +
						hashtable_get_offset(
							&unique_ideals, entry);
			}
		}

		/* sort the sparse rows, append the dense rows,
		   then save the matrix column to disk */

		qsort(mapped_ideals, (size_t)k, 
				sizeof(uint32), compare_uint32);
		c->weight = k;
		for (j = 0; j < dense_row_words; j++)
			mapped_ideals[k++] = dense_rows[j];

		fwrite(mapped_ideals, sizeof(uint32), (size_t)k, matrix_fp);
	}

	/* free temporary structures */

	*nrows = num_dense_rows + hashtable_getall(&unique_ideals, NULL);
	*ncols = num_cycles;
	*dense_rows_out = num_dense_rows;
	*cols_out = cycle_list;

	hashtable_free(&unique_ideals);
	nfs_free_relation_list(rlist, num_relations);
	free(small_ideals);
	free(dense_rows);

	/* reread the matrix columns, now that nothing else
	   except the cycle structures is in memory */

	rewind(matrix_fp);
	for (i = 0; i < num_cycles; i++) {

		la_col_t *c = cycle_list + i;
		uint32 num_words = c->weight + dense_row_words;

		c->data = (uint32 *)malloc(num_words * sizeof(uint32));
		if ( ! c->data ){
			TtyPrintf( 0, "no free memory\n" );
			exit( -1 );
		}
		if (fread(c->data, sizeof(uint32),
				(size_t)num_words, matrix_fp) != num_words) {
			logprintf(obj, "error: matrix file corrupt\n");
			exit(-1);
		}
	}
	fclose(matrix_fp);
	remove(buf);
}

/*--------------------------------------------------------------------*/
static void dump_cycles(msieve_obj *obj, la_col_t *cols, uint32 ncols) {

	uint32 i;
	char buf[256];
	FILE *cycle_fp;

	/* Note that the cycles are deleted after being dumped */

	sprintf(buf, "%s.cyc", obj->savefile_name);
	cycle_fp = fopen(buf, "wb");
	if (cycle_fp == NULL) {
		logprintf(obj, "error: can't open cycle file\n");
		exit(-1);
	}

	fwrite(&ncols, (size_t)1, sizeof(uint32), cycle_fp);

	for (i = 0; i < ncols; i++) {
		la_col_t *c = cols + i;
		uint32 num = c->cycle.num_relations;
		
		c->cycle.list[num - 1] |= (uint32)(0x80000000);
		fwrite(c->cycle.list, (size_t)num, sizeof(uint32), cycle_fp);
		free(c->cycle.list);
	}
	fclose(cycle_fp);
}

/*--------------------------------------------------------------------*/
static void dump_dependencies(msieve_obj *obj, 
			uint64 *deps, uint32 ncols) {

	char buf[256];
	FILE *deps_fp;

	/* we allow up to 64 dependencies, even though the
	   average case will have (64 - POST_LANCZOS_ROWS) */

	sprintf(buf, "%s.dep", obj->savefile_name);
	deps_fp = fopen(buf, "wb");
	if (deps_fp == NULL) {
		logprintf(obj, "error: can't open deps file\n");
		exit(-1);
	}

	fwrite(deps, (size_t)ncols, sizeof(uint64), deps_fp);
	fclose(deps_fp);
}

/*------------------------------------------------------------------*/
void nfs_solve_linear_system(msieve_obj *obj, 
			sieve_param_t *params, mp_t *n) {

	la_col_t *cols;
	uint32 nrows, ncols; 
	uint32 num_dense_rows;
	uint32 deps_found;
	uint64 *dependencies;
	factor_base_t fb;

	logprintf(obj, "\n");
	logprintf(obj, "commencing linear algebra\n");

	/* read in the factor base */

	if (read_factor_base(obj, n, params, &fb)) {
		create_factor_base(obj, &fb, 1);
		write_factor_base(obj, n, params, &fb);
	}

	/* convert the list of relations from the sieving 
	   stage into a matrix. Also free the factor base */

	build_matrix(obj, &fb, &nrows, &num_dense_rows, &ncols, &cols);
	count_matrix_nonzero(obj, nrows, num_dense_rows, ncols, cols);

	/* perform light filtering on the matrix */

	reduce_matrix(obj, &nrows, num_dense_rows, &ncols, cols,
			NUM_EXTRA_RELATIONS);

	if (ncols == 0) {
		logprintf(obj, "matrix is corrupt; skipping linear algebra\n");
		free(cols);
		return;
	}

	/* we've rearranged the list of matrix columns 
	   and have to remember the new order */

	dump_cycles(obj, cols, ncols);

	/* solve the linear system */

	dependencies = block_lanczos(obj, nrows, num_dense_rows,
					ncols, cols, &deps_found);
	if (deps_found)
		dump_dependencies(obj, dependencies, ncols);
	free(dependencies);
	free(cols);
}
