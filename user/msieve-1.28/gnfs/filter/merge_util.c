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

#include "filter.h"
#include "merge_util.h"

/*--------------------------------------------------------------------*/
void ideal_list_init(ideal_list_t *list, 
		uint32 num_ideals, uint32 is_active) {

	uint32 i;

	list->heap_node = (list_t *)malloc(num_ideals * sizeof(list_t));
	list->relset_head = (list_t *)malloc(num_ideals * sizeof(list_t));

	for (i = 0; i < num_ideals; i++) {
		list_t *entry = list->relset_head + i;
		entry->next = entry;
		entry->prev = entry;

		entry = list->heap_node + i;
		entry->payload.ideal_info.num_relsets = 0;
		entry->payload.ideal_info.active = is_active;
		entry->payload.ideal_info.min_relset_size = 0xffff;
		entry->next = entry;
		entry->prev = entry;
	}
}

/*--------------------------------------------------------------------*/
void ideal_list_free(ideal_list_t *list) {

	free(list->heap_node);
	free(list->relset_head);
}

/*--------------------------------------------------------------------*/
void heap_init(heap_t *heap) {

	uint32 i;

	heap->num_bins = 256;
	heap->num_ideals = 0;
	heap->next_bin = heap->num_bins;
	heap->worst_bin = (uint32)(-1);
	heap->hashtable = (list_t *)malloc(heap->num_bins * 
						sizeof(list_t));

	for (i = 0; i < heap->num_bins; i++) {
		list_t *entry = heap->hashtable + i;
		entry->next = entry;
		entry->prev = entry;
	}
}

/*--------------------------------------------------------------------*/
void heap_free(heap_t *heap) {

	free(heap->hashtable);
}

/*--------------------------------------------------------------------*/
static uint32 heap_compute_key(list_t *heap_ideal) {

	/* the heap is used to pick the next ideal to be merged,
	   and the key is the maximum amount of fill-in that can
	   occur when the merge takes place. The key computation
	   implements the Markowitz criterion: merging the least 
	   dense matrix row (of weight c) with r-1 other rows, in 
	   order to eliminate one ideal they all have in common, 
	   will cause at most (r-1)*(c-1) extra nonzero entries to 
	   appear in the matrix. This is a quantitative way of 
	   choosing the ideal to merge that will perturb the current
	   matrix the least, and is a general case of the minimum 
	   degree algorithm.

	   Using a heap to track the Markowitz value of every ideal
	   means that the next ideal to eliminate is chosen in
	   constant time, with no approximations. */

	if (heap_ideal->payload.ideal_info.num_relsets == 0)
		return (uint32)(-1);

	return ((uint32)heap_ideal->payload.ideal_info.num_relsets - 1) *
	       ((uint32)heap_ideal->payload.ideal_info.min_relset_size - 1);
}

/*--------------------------------------------------------------------*/
void heap_add_ideal(heap_t *heap, ideal_list_t *list, uint32 ideal) {

	list_t *head;
	list_t *new_node = list->heap_node + ideal;
	uint32 key = heap_compute_key(new_node);

	if (key == (uint32)(-1)) {
		printf("error: attempted to heapify an empty ideal\n");
		exit(-1);
	}

	/* possibly increase the number of hash buckets in the
	   heap to hold the new key value */

	if (key >= heap->num_bins) {
		uint32 i;
		uint32 new_size = MAX(key + 100, 2 * heap->num_bins);
		list_t *new_hashtable = (list_t *)malloc(new_size *
							sizeof(list_t));

		/* transfer chains of ideals with the same
		   key value over to the new hashtable */
		for (i = 0; i < heap->num_bins; i++) {
			list_t *old_entry = heap->hashtable + i;
			list_t *new_entry = new_hashtable + i;
			new_entry->next = new_entry;
			new_entry->prev = new_entry;
			if (old_entry->next != old_entry) {
				uint32 j;

				j = old_entry->next - list->heap_node;
				new_entry->next = old_entry->next;
				list->heap_node[j].prev = new_entry;

				j = old_entry->prev - list->heap_node;
				new_entry->prev = old_entry->prev;
				list->heap_node[j].next = new_entry;
			}
		}
		
		/* initialize the rest of the entries */

		for (; i < new_size; i++) {
			list_t *entry = new_hashtable + i;
			entry->next = entry;
			entry->prev = entry;
		}
		free(heap->hashtable);
		heap->hashtable = new_hashtable;
		heap->num_bins = new_size;
	}

	/* add the new ideal */

	head = heap->hashtable + key;
	new_node->prev = head;
	new_node->next = head->next;
	head->next->prev = new_node;
	head->next = new_node;

	/* adjust the best and worst heap bucket pointers */

	heap->num_ideals++;
	heap->next_bin = MIN(heap->next_bin, key);
	if (heap->worst_bin == (uint32)(-1))
		heap->worst_bin = key;
	else
		heap->worst_bin = MAX(heap->worst_bin, key);
}
	

/*--------------------------------------------------------------------*/
void heap_remove_ideal(heap_t *heap, ideal_list_t *list, 
				uint32 ideal) {

	list_t *node = list->heap_node + ideal;
	uint32 key = heap_compute_key(node);
	list_t *head;

	if (node->next == node)
		return;

	/* remove the ideal from the chain of ideals with
	   this key value */

	heap->num_ideals--;
	node->next->prev = node->prev;
	node->prev->next = node->next;
	node->prev = node;
	node->next = node;

	/* adjust the best and worst heap bucket pointers */

	head = heap->hashtable + heap->next_bin;
	if (head->next == head) {
		uint32 i = key;
		while (i < heap->num_bins && head->next == head) {
			head++;
			i++;
		}
		heap->next_bin = i;
	}

	head = heap->hashtable + heap->worst_bin;
	if (head->next == head) {
		uint32 i = key;
		while ((int32)i >= 0 && head->next == head) {
			head--;
			i--;
		}
		heap->worst_bin = i;
	}
}

/*--------------------------------------------------------------------*/
uint32 heap_add_relset(heap_t *active_heap, 
			heap_t *inactive_heap,
			ideal_list_t *list, 
			relation_set_t *r,
			uint32 min_ideal_weight) {
	
	uint32 i;
	uint32 weight = r->num_small_ideals + r->num_large_ideals;

	r->num_active_ideals = 0;

	/* add the relation set by adding each of its 
	   large ideals to the heap */

	for (i = 1; i <= r->num_large_ideals; i++) {
		list_t *r_ideal = r->ideal_list + i;
		uint32 ideal = r_ideal->payload.ideal;
		list_t *relset_head = list->relset_head + ideal;
		list_t *heap_node = list->heap_node + ideal;
		uint32 active = heap_node->payload.ideal_info.active;
		heap_t *heap = active ? active_heap : inactive_heap;
		uint16 prev_weight = 
				heap_node->payload.ideal_info.min_relset_size;

		/* pull ideal i off the heap temporarily */

		heap_remove_ideal(heap, list, ideal);

		/* update its statistics */

		heap_node->payload.ideal_info.num_relsets++;
		heap_node->payload.ideal_info.min_relset_size = 
						MIN(prev_weight, weight);

		/* add the relation set to the list of relation
		   sets containing ideal i */

		r_ideal->prev = relset_head;
		r_ideal->next = relset_head->next;
		relset_head->next->prev = r_ideal;
		relset_head->next = r_ideal;

		/* if there are enough relation sets still 
		   containing ideal i, put it back into the heap */

		if (heap_node->payload.ideal_info.num_relsets > 
					min_ideal_weight) {
			heap_add_ideal(heap, list, ideal);
		}

		if (active)
			r->num_active_ideals++;
	}

	/* return the number of ideals that still 
	   need merging before r can become a cycle */

	return r->num_active_ideals;
}

/*--------------------------------------------------------------------*/
void heap_remove_relset(heap_t *active_heap, 
			heap_t *inactive_heap,
			ideal_list_t *list,
			relation_set_t *r,
			relation_set_t *relset_array,
			uint32 min_ideal_weight) {
	
	uint32 i;
	uint32 weight = r->num_small_ideals + r->num_large_ideals;

	/* remove the relation set by removing each of its 
	   large ideals from the heap */

	for (i = 1; i <= r->num_large_ideals; i++) {
		list_t *r_ideal = r->ideal_list + i;
		uint32 ideal = r_ideal->payload.ideal;
		list_t *relset_head = list->relset_head + ideal;
		list_t *heap_node = list->heap_node + ideal;
		uint32 active = heap_node->payload.ideal_info.active;
		heap_t *heap = active ? active_heap : inactive_heap;
		uint16 prev_weight = 
				heap_node->payload.ideal_info.min_relset_size;

		/* pull ideal i off the heap temporarily */

		heap_remove_ideal(heap, list, ideal);

		/* remove the relation set from the list of 
		   relation sets containing ideal i */

		r_ideal->prev->next = r_ideal->next;
		r_ideal->next->prev = r_ideal->prev;
		r_ideal->next = r_ideal;
		r_ideal->prev = r_ideal;

		if (--(heap_node->payload.ideal_info.num_relsets) == 0) {

			/* no more relations with this ideal
			   left in the heap */

			heap_node->payload.ideal_info.min_relset_size = 0xffff;
		}
		else {
			/* if r was the lightest relation set, we need 
			   to update the minimum relation set weight for
			   ideal i */

			if (weight == prev_weight) {
				list_t *curr_relset = relset_head->next;
				uint32 curr_weight = 0xffff;
				while (curr_relset != relset_head) {
					list_t *tmp = curr_relset;
					relation_set_t *new_r;
					uint32 new_weight;
					while (tmp->next != NULL)
						tmp--;
					new_r = relset_array + 
						tmp->payload.relset_num;
					new_weight = new_r->num_small_ideals +
						     new_r->num_large_ideals;
					curr_weight = MIN(curr_weight, 
							  new_weight);
					curr_relset = curr_relset->next;
				}
				heap_node->payload.ideal_info.min_relset_size =
							curr_weight;
			}

			/* if there are enough relation sets still
			   containing ideal i, put it back into the heap */

			if (heap_node->payload.ideal_info.num_relsets > 
							min_ideal_weight) {
				heap_add_ideal(heap, list, ideal);
			}
		}
	}
}

/*--------------------------------------------------------------------*/
uint32 heap_remove_best(heap_t *heap, ideal_list_t *list) {

	/* remove the heap element with the smallest
	   Markowitz value */

	uint32 ideal;
	uint32 key = heap->next_bin;

	if (key == heap->num_bins)
		return (uint32)(-1);
	
	ideal = heap->hashtable[key].next - list->heap_node;
	heap_remove_ideal(heap, list, ideal);
	return ideal;
}
	
/*--------------------------------------------------------------------*/
uint32 heap_remove_worst(heap_t *heap, ideal_list_t *list) {

	/* remove the heap element with the largest
	   Markowitz value */

	uint32 ideal;
	uint32 key = heap->worst_bin;

	if ((int32)key < 0)
		return (uint32)(-1);
	
	ideal = heap->hashtable[key].next - list->heap_node;
	heap_remove_ideal(heap, list, ideal);
	return ideal;
}
	
/*--------------------------------------------------------------------*/
void load_next_relset_group(merge_aux_t *aux,
			heap_t *active_heap, heap_t *inactive_heap, 
			ideal_list_t *ideal_list,
			relation_set_t *relset_array,
			uint32 ideal,
			uint32 min_ideal_weight) {

	uint32 i;
	list_t *relset_head;
	list_t *r;

	/* make aux ready to receive a batch of relation sets */

	aux->num_relsets = ideal_list->heap_node[
				ideal].payload.ideal_info.num_relsets;
	if (aux->num_relsets >= MERGE_MAX_OBJECTS) {
		printf("error: number of relsets too large\n");
		exit(-1);
	}

	/* copy each relation set containing 'ideal', and
	   remember the index into the full array of
	   relation sets where these occur. We'll need this
	   information in order to put everything back when
	   processing of aux has finished */

	relset_head = ideal_list->relset_head + ideal;
	r = relset_head->next;
	i = 0;

	while (r != relset_head) {
		list_t *tmp = r;

		while (tmp->next != NULL)
			tmp--;

		aux->tmp_relset_idx[i] = tmp->payload.relset_num;
		aux->tmp_relsets[i] = 
				relset_array[tmp->payload.relset_num];
		i++;
		r = r->next;
	}

	if (i != aux->num_relsets) {
		printf("error: relation list is corrupt\n");
		exit(-1);
	}

	/* now that all the relation sets are safely copied,
	   go back and remove the originals from the heap,
	   wiping out the old relation set structures in
	   the process */

	for (i = 0; i < aux->num_relsets; i++) {
		relation_set_t *curr_r = relset_array + 
					aux->tmp_relset_idx[i];
		heap_remove_relset(active_heap, inactive_heap,
				ideal_list, curr_r, relset_array,
				min_ideal_weight);
		curr_r->relation_list = NULL;
		curr_r->ideal_list = NULL;
	}
}

/*--------------------------------------------------------------------*/
void bury_inactive_ideal(relation_set_t *relset_array,
			ideal_list_t *ideal_list, uint32 ideal) {

	list_t *head = ideal_list->relset_head + ideal;
	list_t *r = head->next;

	while (r != head) {

		list_t *tmp = r;
		relation_set_t *curr_r;
		uint32 num_ideals;

		/* point to the next relset early, since the
		   pointer is about to get overwritten */

		r = r->next;
		while (tmp->next != NULL)
			tmp--;

		/* remove ideal from the list of ideals in curr_r */

		curr_r = relset_array + tmp->payload.relset_num;
		curr_r->num_small_ideals++;
		num_ideals = curr_r->num_large_ideals;
		if (--(curr_r->num_large_ideals) == 0) {
			free(curr_r->ideal_list);
			curr_r->ideal_list = NULL;
		}
		else {
			uint32 i;
			list_t *ideal_list = curr_r->ideal_list;

			/* squeeze out the list_t containing ideal.
			   Because the total weight of the relation
			   set is unchanged, we don't have to re-heapify
			   any of the other ideals in curr_r */

			for (i = 1; i <= num_ideals; i++) {
				if (ideal_list[i].payload.ideal == ideal)
					break;
			}
			for (i++; i <= num_ideals; i++) {
				list_t *moved_ideal = ideal_list + i - 1;
				*moved_ideal = ideal_list[i];
				moved_ideal->prev->next = moved_ideal;
				moved_ideal->next->prev = moved_ideal;
			}
		}
	}
}
