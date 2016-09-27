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

/* implementation of number field sieve filtering */

#ifndef _FILTER_H_
#define _FILTER_H_

#include <common.h>
#include "gnfs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* the singleton removal phase uses a packed representation
   of each relation. The following maps relations to large ideals;
   the ideals themselves don't matter, only the unique number
   that each is assigned */

typedef struct {
	uint32 rel_index;      /* savefile line number where relation occurs */
	uint8 ideal_count;     /* number of large ideals */
	uint8 gf2_factors;     /* number of small factors that are ignored */
	uint8 connected;       /* scratch space used in clique formation */
	uint32 ideal_list[TEMP_FACTOR_LIST_SIZE];  /* the large ideals */
} relation_ideal_t;

/* relations can have between 0 and TEMP_FACTOR_LIST_SIZE 
   large ideals, and the average number is very small (2 or 3). 
   When processing large numbers of relations we do not 
   assume a full relation_ideal_t struct is allocated for each. 
   Instead all such structures are packed together as
   tightly as possible, and we always iterate sequentially
   through the array. The following abstracts away the process
   of figuring out the number of bytes used in a current
   relation_ideal_t in the list, then pointing beyond it */

static INLINE relation_ideal_t *next_relation_ptr(relation_ideal_t *r) {
	return (relation_ideal_t *)((uint8 *)r +
			sizeof(relation_ideal_t) - 
			sizeof(r->ideal_list[0]) * 
			(TEMP_FACTOR_LIST_SIZE - r->ideal_count));
}

/* structure for the mapping between large ideals 
   and relations (used during clique removal) */

typedef struct {
	uint32 relation_array_word;  /* 32-bit word offset into relation
					array where the relation starts */
	uint32 next;		     /* next relation containing this ideal */
} ideal_relation_t;

/* structure used to map between a large ideal and a 
   linked list of relations that use that ideal */

typedef struct {
	uint32 payload : 30;	/* offset in list of ideal_relation_t
				   structures where the linked list of
				   ideal_relation_t's for this ideal starts */
	uint32 clique : 1;      /* nonzero if this ideal can participate in
				   a clique */
	uint32 connected : 1;   /* nonzero if this ideal has already been
				   added to a clique under construction */
} ideal_map_t;

/* main structure controlling relation filtering */

typedef struct {
	relation_ideal_t *relation_array;  /* relations after singleton phase */
	uint32 num_relations;       /* current number of relations */
	uint32 num_ideals;          /* current number of unique large ideals */
	uint32 max_ideal_degree;    /* largest number of relations in which
					an ideal occurs */
	uint32 filtmin_r;           /* min. value a rational ideal needs to
					to be tracked during filter phase */
	uint32 filtmin_a;          /* min. value an algebraic ideal needs to
					to be tracked during filter phase */
	uint32 target_excess;      /* how many more relations than ideals
					are required for filtering to proceed */
} filter_t;

/* merging of relations into cycles needs circular linked
   lists for several different purposes. All use this structure */

typedef struct list_t {
	union {
		uint32 ideal;			/* value of large ideal */
		uint32 relset_num;		/* index of relation set */
		struct {
			uint32 num_relsets : 15;/* the number of relation sets
						   containing this ideal */
			uint32 active : 1;      /* 1 if ideal is active */
			uint16 min_relset_size; /* the number of ideals in the
						   lightest relation set that
						   contains this ideal */
		} ideal_info;
	} payload;
	struct list_t *next;
	struct list_t *prev;
} list_t;

/* representation of a 'relation set', i.e. a group
   of relations that will all participate in the same
   column when the final matrix is formed */

typedef struct {
	uint16 num_relations;     /* number of relations in this relation set */
	uint16 num_small_ideals;  /* number of ideals that are not tracked */
	uint16 num_large_ideals;  /* number of ideals that are tracked */
	uint16 num_active_ideals; /* number of ideals eligible for merging
				     (0 means relset is a cycle) */
	uint32 *relation_list;    /* list of the relation numbers of all the
				     relations paticipating in this set */
	list_t *ideal_list;       /* list of large ideals to process. Element
				     0 acts as a reverse pointer back to the
				     relation_set_t owning the list of ideals,
				     so there are num_large_ideals+1 list
				     elements */
} relation_set_t;

/* structure controlling the merge process */

typedef struct {
	uint32 num_relsets;           /* current number of relation sets */
	uint32 num_ideals;            /* number of unique ideals that must
					 be merged */
	double avg_cycle_weight;      /* the avg number of nonzeros per cycle */
	relation_set_t *relset_array; /* current list of relation sets */
} merge_t;


/* create '<savefile_name>.d', a binary file containing
   the line numbers of unique relations. The return value is
   the large prime bound to use for the rest of the filtering */

uint32 nfs_purge_duplicates(msieve_obj *obj, factor_base_t *fb); 

/* read '<savefile_name>.d' and create '<savefile_name>.s', a 
   binary file containing the line numbers of relations that
   are not singletons. This also performs clique processing
   and fills in 'filter' with the large ideals of surviving 
   relations before writing the file.

   If disk_based is zero, the process begins instead from
   <savefile_name>.s; this allows singleton removal and clique 
   processing to refine a previously processed dataset but 
   with new filtering parameters 
   
   The return value is zero if all of this succeeds. The only
   reason it would fail is if there are not enough relations in
   the original dataset for filtering to proceed. In that case,
   an estimate is returned of the number of additional relations
   needed so that singleton removal is likely to succeed */

uint32 nfs_purge_singletons(msieve_obj *obj, factor_base_t *fb,
			filter_t *filter, uint32 disk_based);

/* perform clique removal on the current set of relations.
   see clique.c for explanations of the last two parameters */

uint32 nfs_purge_cliques(msieve_obj *obj, filter_t *filter,
			uint32 clique_heap_size, 
			uint32 excess_relations); 

/* initialize the merge process */

void nfs_merge_init(msieve_obj *obj, filter_t *filter); 

/* perform all 2-way merges, converting the results into
   relation-sets that the main merge routine operates on */

void nfs_merge_2way(msieve_obj *obj, filter_t *filter, merge_t *merge);

/* do the rest of the merging. min_cycles is the minimum number
   of cycles that the input collection of relation-sets must
   produce, corresponding to the smallest matrix that can be
   built (the actual matrix is expected to be much larger than 
   this). */

void nfs_merge_full(msieve_obj *obj, merge_t *merge, uint32 min_cycles);

/* perform post-processing optimizations on the collection of cycles
   found by the merge phase */

void nfs_merge_post(msieve_obj *obj, merge_t *merge);

#ifdef __cplusplus
}
#endif

#endif /* _FILTER_H_ */
