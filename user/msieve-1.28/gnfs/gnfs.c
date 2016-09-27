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

/* The first few parameter sets (except the sieve length) are
   from GGNFS. That doesn't mean they're right for this
   implementation though. The line length is a total guess */

static sieve_param_t prebuilt_params[] = {
	{MIN_NFS_BITS, 1800000, 1800000, 1<<26, 1<<26, 48, 48, 4000000},
	{342,          2300000, 2300000, 1<<26, 1<<26, 49, 49, 8000000},
	{352,          2500000, 2500000, 1<<26, 1<<26, 49, 49, 12000000},
	{365,          3200000, 3200000, 1<<27, 1<<27, 50, 50, 16000000},
	{372,          3500000, 3500000, 1<<27, 1<<27, 50, 50, 20000000},
	{392,          4500000, 4500000, 1<<27, 1<<27, 50, 50, 24000000},
	{405,          5000000, 5000000, 1<<27, 1<<27, 50, 50, 28000000},
	{419,          5400000, 5400000, 1<<27, 1<<27, 51, 51, 32000000},

	/* Compared to published sets of parameters for 512 bit input, 
	   the following are highly suboptimal */

	{433,          5800000, 5800000, 1<<28, 1<<28, 51, 51, 36000000},
	{447,          6200000, 6200000, 1<<28, 1<<28, 52, 52, 40000000},
	{463,          6600000, 6600000, 1<<28, 1<<28, 52, 52, 44000000},
	{477,          7000000, 7000000, 1<<29, 1<<29, 53, 53, 48000000},
	{492,          7400000, 7400000, 1<<29, 1<<29, 53, 53, 52000000},
	{506,          7800000, 7800000, 1<<30, 1<<30, 54, 54, 56000000},
	{520,          8200000, 8200000, 1<<30, 1<<30, 54, 54, 60000000},
};

static void get_sieve_params(uint32 bits, sieve_param_t *params);

static uint32 init_savefile(msieve_obj *obj, mp_t *n, 
				uint32 *have_free_relations);

/*--------------------------------------------------------------------*/
uint32 factor_gnfs(msieve_obj *obj, mp_t *n,
			factor_list_t *factor_list) {

	int32 status;
	uint32 bits;
	sieve_param_t params;
	prime_list_t prime_list;
	mp_poly_t rat_poly;
	mp_poly_t alg_poly;
	uint32 have_free_relations = 0;
	uint32 relations_found = 0;
	uint32 max_relations = 0;
	uint32 factor_found = 0;
	mp_t reduced_n;

	/* Calculate the factor base bound */

	bits = mp_bits(n);
	get_sieve_params(bits, &params);

	logprintf(obj, "commencing number field sieve (%d-digit input)\n",
			(uint32)((double)bits / 3.3219 + 0.5));

	/* Make a prime list, trial factor using it (if the factor
	   base bound exceeds the library's ordinary trial factoring
	   bound). If a factor is found, give up; the remaining
	   cofactor could be a power or could be more efficiently
	   factored with other methods */

	fill_prime_list(&prime_list, 0xffffffff, 
			MAX(params.rfb_limit, params.afb_limit));
	status = trial_factor(obj, n, &reduced_n,
				&prime_list, factor_list);
	free(prime_list.list);
	if (status)
		return 1;

	/* generate or read in the NFS polynomials */

	memset(&rat_poly, 0, sizeof(rat_poly));
	memset(&alg_poly, 0, sizeof(alg_poly));
	status = read_poly(obj, n, &rat_poly, &alg_poly);
	if (status != 0 && (obj->flags & MSIEVE_FLAG_NFS_POLY)) {
		status = find_poly(obj, n);
		status = read_poly(obj, n, &rat_poly, &alg_poly);
	}
	if (status != 0) {
		printf("error generating or reading NFS polynomials\n");
		return 0;
	}
	analyze_one_poly(obj, &rat_poly, &alg_poly);

	if ((obj->flags & MSIEVE_FLAG_STOP_SIEVING) ||
	    !(obj->flags & (MSIEVE_FLAG_NFS_SIEVE |
	    		    MSIEVE_FLAG_NFS_FILTER |
			    MSIEVE_FLAG_NFS_LA |
			    MSIEVE_FLAG_NFS_SQRT))) {
		return 0;
	}

	/* if we're supposed to be sieving, 
	   initialize the savefile */

	if (obj->flags & (MSIEVE_FLAG_NFS_SIEVE |
			  MSIEVE_FLAG_NFS_FILTER)) {

		/* figure out how many relations to look for, and 
		   quit early if that many have already been found */

		relations_found = init_savefile(obj, n, &have_free_relations);
		if (obj->max_relations > 0) {
			max_relations = relations_found + 
						obj->max_relations;
		}
		else {
			/* if no guidance on this is available, make a
			   wild guess: a fixed fraction of the total number 
			   of large primes possible */

			max_relations = 0.5 * (params.rfb_lp_size /
					log((double)params.rfb_lp_size) +
					params.afb_lp_size /
					log((double)params.afb_lp_size));
		}
	}

	/* this is a little tricky: perform only sieving (and
	   quit when enough relations are found), only filtering
	   (and quit if it fails, or continue the postprocessing
	   if it succeeds), or alternate between sieving and
	   filtering until the filtering succeeds */

	while (1) {
		if (!(obj->flags & (MSIEVE_FLAG_NFS_SIEVE |
				    MSIEVE_FLAG_NFS_FILTER))) {
			break;
		}

		if (obj->flags & MSIEVE_FLAG_NFS_SIEVE) {
			obj->savefile = fopen(obj->savefile_name, "a+");
			relations_found = do_line_sieving(obj, &params, n, 
							relations_found,
							max_relations);
			fclose(obj->savefile);
			obj->savefile = NULL;
			if (relations_found == 0)
				break;
			if (!(obj->flags & MSIEVE_FLAG_NFS_FILTER))
				return 0;
		}

		if (obj->flags & MSIEVE_FLAG_STOP_SIEVING)
			return 0;

		if (obj->flags & MSIEVE_FLAG_NFS_FILTER) {

			/* if the savefile does not contain free relations,
			   add them. We do this only if filtering is specified,
			   to prevent multiple sieving machines from adding
			   free relations multiple times */

			if (!have_free_relations) {
				obj->savefile = fopen(obj->savefile_name, "a+");
				relations_found += add_free_relations(
							obj, &params, n);
				fclose(obj->savefile);
				obj->savefile = NULL;
				have_free_relations = 1;
			}

			max_relations = nfs_filter_relations(obj, n);
			if (max_relations == 0)
				break;
			logprintf(obj, "filtering wants %u more relations\n",
							max_relations);
			if (!(obj->flags & MSIEVE_FLAG_NFS_SIEVE))
				return 0;
			max_relations += relations_found;
		}
	}

	if (obj->flags & MSIEVE_FLAG_NFS_LA)
		nfs_solve_linear_system(obj, &params, n);
		
	if (obj->flags & MSIEVE_FLAG_NFS_SQRT) {
		factor_found = nfs_find_factors(obj, n, factor_list);
	}
	return factor_found;
}

/*--------------------------------------------------------------------*/
static void get_sieve_params(uint32 bits, sieve_param_t *params) {

	sieve_param_t *low, *high;
	uint32 max_size;
	uint32 i, j, dist;

	/* For small inputs, use the first set of precomputed
	   parameters */

	if (bits < prebuilt_params[0].bits) {
		*params = prebuilt_params[0];
		return;
	}

	/* bracket the input size between two table entries */

	max_size = sizeof(prebuilt_params) / sizeof(sieve_param_t);
	if (bits >= prebuilt_params[max_size - 1].bits) {
		*params = prebuilt_params[max_size - 1];
		return;
	}

	/* if the input is too large, just use the last table entry.
	   This means that the choice of parameters is increasingly
	   inappropriate as the input becomes larger, but there's no
	   guidance on what to do in this case anyway. */

	for (i = 0; i < max_size - 1; i++) {
		if (bits < prebuilt_params[i+1].bits)
			break;
	}

	/* Otherwise the parameters to use are a weighted average 
	   of the two table entries the input falls between */

	low = &prebuilt_params[i];
	high = &prebuilt_params[i+1];
	dist = high->bits - low->bits;
	i = bits - low->bits;
	j = high->bits - bits;

	params->bits = bits;
	params->rfb_limit = (uint32)(
			 ((double)low->rfb_limit * j +
			  (double)high->rfb_limit * i) / dist + 0.5);
	params->afb_limit = (uint32)(
			 ((double)low->afb_limit * j +
			  (double)high->afb_limit * i) / dist + 0.5);
	params->rfb_lp_size = (uint32)(
			 ((double)low->rfb_lp_size * j +
			  (double)high->rfb_lp_size * i) / dist + 0.5);
	params->afb_lp_size = (uint32)(
			 ((double)low->afb_lp_size * j +
			  (double)high->afb_lp_size * i) / dist + 0.5);
	params->rfb_lp_cutoff_bits = (uint32)(
			 ((double)low->rfb_lp_cutoff_bits * j +
			  (double)high->rfb_lp_cutoff_bits * i) / dist + 0.5);
	params->afb_lp_cutoff_bits = (uint32)(
			 ((double)low->afb_lp_cutoff_bits * j +
			  (double)high->afb_lp_cutoff_bits * i) / dist + 0.5);
	params->sieve_size = (uint64)(
			 ((double)low->sieve_size * j +
			  (double)high->sieve_size * i) / dist + 0.5);
}

/*--------------------------------------------------------------------*/
void eval_poly(signed_mp_t *res, int64 a, uint32 b, mp_poly_t *poly) {

	/* Evaluate one polynomial at 'a' and 'b' */

	mp_t power, tmp;
	uint32 rsign, asign, csign;
	uint64 abs_a;
	mp_t mp_abs_a;
	int32 d = poly->degree;
	int32 comparison;

	power.nwords = power.val[0] = 1;
	abs_a = a;
	asign = POSITIVE;
	if (a < 0) {
		abs_a = -a;
		asign = NEGATIVE;
	}

	mp_abs_a.val[0] = (uint32)abs_a;
	mp_abs_a.val[1] = (uint32)(abs_a >> 32);
	mp_abs_a.nwords = 0;
	if (mp_abs_a.val[1])
		mp_abs_a.nwords = 2;
	else if (mp_abs_a.val[0])
		mp_abs_a.nwords = 1;

	mp_mul(&poly->coeff[d].num, &mp_abs_a, &res->num);
	rsign = poly->coeff[d].sign ^ asign;

	while (--d >= 0) {
		mp_mul_1(&power, b, &power);
		mp_mul(&power, &poly->coeff[d].num, &tmp);
		csign = poly->coeff[d].sign;

		switch (2 * rsign + csign) {
		case 0:
		case 3:
			mp_add(&res->num, &tmp, &tmp);
			break;
		case 1:
			comparison = mp_cmp(&res->num, &tmp);
			if (comparison > 0) {
				mp_sub(&res->num, &tmp, &tmp);
			}
			else {
				mp_sub(&tmp, &res->num, &tmp);
				if (!mp_is_zero(&tmp))
					rsign = NEGATIVE;
			}
			break;
		case 2:
			comparison = mp_cmp(&res->num, &tmp);
			if (comparison > 0) {
				mp_sub(&res->num, &tmp, &tmp);
			}
			else {
				mp_sub(&tmp, &res->num, &tmp);
				rsign = POSITIVE;
			}
			break;
		}

		if (d > 0) {
			mp_mul(&tmp, &mp_abs_a, &res->num);
			rsign = rsign ^ asign;
		}
		else {
			mp_copy(&tmp, &res->num);
		}
	}
	res->sign = rsign;
}

/*--------------------------------------------------------------------*/
void hashtable_init(hashtable_t *h, uint32 log2_hashtable_size,
		    uint32 init_match_size, uint32 blob_words) {

	h->hashtable = (uint32 *)calloc((size_t)1 << log2_hashtable_size,
					sizeof(uint32));
	if ( ! h->hashtable ){
		TtyPrintf( 0, "no free memory\n" );
		exit( -1 );
	}
	h->match_array = (hash_t *)malloc(init_match_size * sizeof(hash_t));
	if ( ! h->match_array ){
		TtyPrintf( 0, "no free memory\n" );
		exit( -1 );
	}
	h->match_array_alloc = init_match_size;
	h->match_array_size = 1;
	h->log2_hashtable_size = log2_hashtable_size;
	h->blob_words = blob_words;
}

void hashtable_free(hashtable_t *h) {
	free(h->hashtable);
	free(h->match_array);
	h->hashtable = NULL;
	h->match_array = NULL;
}

void hashtable_close(hashtable_t *h) {
	free(h->hashtable);
	h->hashtable = NULL;

	/* trim down the list that is currently allocated */

	h->match_array = (hash_t *)realloc(h->match_array,
				h->match_array_size * sizeof(hash_t));
	if ( ! h->match_array ){
		TtyPrintf( 0, "no free memory\n" );
		exit( -1 );
	}
	h->match_array_alloc = h->match_array_size;
}

uint32 hashtable_getall(hashtable_t *h, hash_t **all_entries) {
	if (all_entries) {
		*all_entries = h->match_array + 1;
	}
	return h->match_array_size - 1;
}

uint32 hashtable_get_offset(hashtable_t *h, hash_t *entry) {
	return (entry - h->match_array) - 1;
}

hash_t *hashtable_find(hashtable_t *h, void *blob, uint32 *present) {

	uint32 i;
	uint32 offset, hashval;
	uint32 *key = (uint32 *)blob;
	hash_t *entry = NULL;
	uint32 *hashtable = h->hashtable;
	hash_t *match_array = h->match_array;
	uint32 log2_hashtable_size = h->log2_hashtable_size;
	uint32 blob_words = h->blob_words;

	/* pre-emptively increase the size allocated
	   for hashtable matches */
	
	if (h->match_array_size + 1 >= h->match_array_alloc) {
		h->match_array_alloc *= 2;
		match_array = h->match_array = (hash_t *)realloc(
						h->match_array,
						h->match_array_alloc *
						sizeof(hash_t));
		if ( ! match_array ){
			TtyPrintf( 0, "no free memory\n" );
			exit( -1 );
		}
	}

	/* compute hash value */

	hashval = HASH1(key[0]);
	if (blob_words > 1)
		hashval ^= HASH2(key[1]);
	hashval = hashval >> (32 - log2_hashtable_size);

	/* attempt to find it in the table */

	offset = hashtable[hashval];
	while (offset != 0) {
		entry = match_array + offset;
		for (i = 0; i < blob_words; i++) {
			if (entry->payload[i] != key[i])
				break;
		}
		if (i == blob_words)
			break;
		offset = entry->next;
	}

	/* not found; add it */

	if (offset == 0) {
		entry = match_array + h->match_array_size;
		entry->next = hashtable[hashval];
		for (i = 0; i < blob_words; i++) {
			entry->payload[i] = key[i];
		}
		hashtable[hashval] = h->match_array_size++;
	}
	if (present)
		*present = offset;

	return entry;
}

/*--------------------------------------------------------------------*/
void nfs_print_to_savefile(msieve_obj *obj, char *buf) {

	if (obj->savefile_buf_off + strlen(buf) + 1 >= SAVEFILE_BUF_SIZE) {
		fprintf(obj->savefile, "%s", obj->savefile_buf);
		fflush(obj->savefile);
		obj->savefile_buf_off = 0;
	}

	obj->savefile_buf_off += sprintf(obj->savefile_buf + 
				obj->savefile_buf_off, "%s", buf);
}

/*--------------------------------------------------------------------*/
void nfs_flush_savefile(msieve_obj *obj) {

	fprintf(obj->savefile, "%s", obj->savefile_buf);
	fflush(obj->savefile);
	obj->savefile_buf_off = 0;
	obj->savefile_buf[0] = 0;
}

/*------------------------------------------------------------------*/
static uint32 init_savefile(msieve_obj *obj, mp_t *n,
				uint32 *free_rel) {

	uint32 relations_found = 0;
	FILE *savefile;

	/* open the savefile; if the file already
	   exists and the first line contains n,
	   then count the relations that have already
	   been found */

	savefile = fopen(obj->savefile_name, "a+");
	if (savefile == NULL) {
		printf("error: cannot open savefile '%s'\n",
				obj->savefile_name);
		exit(-1);
	}

	*free_rel = 0;
	fseek(savefile, (long)0, SEEK_END);
	if (ftell(savefile) != 0) {
		char buf[256];
		mp_t read_n;

		rewind(savefile);
		fgets(buf, (int)sizeof(buf), savefile);
		mp_clear(&read_n);
		if (buf[0] == 'N')
			mp_str2mp(buf + 2, &read_n, 10);
		if (mp_cmp(n, &read_n) != 0) {

			/* the savefile was for a different n. Truncate
			   the file and write the present n. I hope you
			   backed up savefiles you wanted! */

			fclose(savefile);
			savefile = fopen(obj->savefile_name, "w");
			nfs_print_to_savefile(obj, "N ");
			nfs_print_to_savefile(obj, mp_sprintf(n, 
					10, obj->mp_sprintf_buf));
			nfs_print_to_savefile(obj, "\n");
		}
		else {
			/* count relations; No checking of relations 
			   happens here */

			fgets(buf, (int)sizeof(buf), savefile);

			while (!feof(savefile)) {
				if (isdigit(buf[0]) || buf[0] == '-') {
					relations_found++;
					if (*free_rel == 0) {
						char *b = strchr(buf, ',');
						if (b != NULL) {
							if (atol(b + 1) == 0)
								*free_rel = 1;
						}
					}
				}
				fgets(buf, (int)sizeof(buf), savefile);
			}
			fseek(savefile, (long)0, SEEK_END);

			logprintf(obj, "restarting with %u relations\n",
						relations_found);
		}
	}
	else {
		/* start a new savefile; write n to it */

		nfs_print_to_savefile(obj, "N ");
		nfs_print_to_savefile(obj, mp_sprintf(n, 10, 
					obj->mp_sprintf_buf));
		nfs_print_to_savefile(obj, "\n");
	}

	fclose(savefile);
	return relations_found;
}
