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

#include "lanczos.h"

/*-------------------------------------------------------------------*/
static void mul_unpacked(packed_matrix_t *matrix,
			  uint64 *x, uint64 *b) {

	uint32 ncols = matrix->ncols;
	uint32 num_dense_rows = matrix->num_dense_rows;
	la_col_t *A = matrix->unpacked_cols;
	uint32 i, j;

	memset(b, 0, ncols * sizeof(uint64));
	
	for (i = 0; i < ncols; i++) {
		la_col_t *col = A + i;
		uint32 *row_entries = col->data;
		uint64 tmp = x[i];

		for (j = 0; j < col->weight; j++) {
			b[row_entries[j]] ^= tmp;
		}
	}

	if (num_dense_rows) {
		for (i = 0; i < ncols; i++) {
			la_col_t *col = A + i;
			uint32 *row_entries = col->data + col->weight;
			uint64 tmp = x[i];
	
			for (j = 0; j < num_dense_rows; j++) {
				if (row_entries[j / 32] & 
						((uint32)1 << (j % 32))) {
					b[j] ^= tmp;
				}
			}
		}
	}
}

/*-------------------------------------------------------------------*/
static void mul_trans_unpacked(packed_matrix_t *matrix,
				uint64 *x, uint64 *b) {

	uint32 ncols = matrix->ncols;
	uint32 num_dense_rows = matrix->num_dense_rows;
	la_col_t *A = matrix->unpacked_cols;
	uint32 i, j;

	for (i = 0; i < ncols; i++) {
		la_col_t *col = A + i;
		uint32 *row_entries = col->data;
		uint64 accum = 0;

		for (j = 0; j < col->weight; j++) {
			accum ^= x[row_entries[j]];
		}
		b[i] = accum;
	}

	if (num_dense_rows) {
		for (i = 0; i < ncols; i++) {
			la_col_t *col = A + i;
			uint32 *row_entries = col->data + col->weight;
			uint64 accum = b[i];
	
			for (j = 0; j < num_dense_rows; j++) {
				if (row_entries[j / 32] &
						((uint32)1 << (j % 32))) {
					accum ^= x[j];
				}
			}
			b[i] = accum;
		}
	}
}

/*-------------------------------------------------------------------*/
static void mul_one_med_block(packed_block_t *curr_block,
			uint64 *curr_col, uint64 *curr_b) {

	uint16 *entries = curr_block->med_entries;

	while (1) {
		uint32 i = 0;
		uint32 row = entries[0];
		uint32 count = entries[1];
		uint64 accum;

		if (count == 0)
			break;

		/* Unlike the sparse blocks, medium-dense blocks
		   have enough entries that they can be stored in
		   row-major order, with many entries in each row.
		   One iteration of the while loop handles an entire
		   row at a time */

		/* curr_col and curr_b are both cached, so we have to
		   minimize the number of memory accesses and calculate
		   pointers as early as possible */

#if defined(__GNUC__) && defined(__i386__) && defined(HAS_MMX)
		#define _txor(k)				\
			"movzwl %%ax, %%edx		\n\t"	\
			"pxor (%2,%%edx,8), %0		\n\t"	\
			"shrl $16, %%eax		\n\t"	\
			"pxor (%2,%%eax,8), %%mm0	\n\t"	\
			"movl 2*(2+4+(" #k "))(%3,%4,2), %%eax \n\t"	\
			"movzwl %%cx, %%edx		\n\t"	\
			"pxor (%2,%%edx,8), %0		\n\t"	\
			"shrl $16, %%ecx		\n\t"	\
			"pxor (%2,%%ecx,8), %%mm0	\n\t"	\
			"movl 2*(2+6+(" #k "))(%3,%4,2), %%ecx \n\t"

		asm volatile(
			"movl 2*(2+0)(%3,%4,2), %%eax	\n\t"
			"movl 2*(2+2)(%3,%4,2), %%ecx	\n\t"
			"pxor %0, %0			\n\t"
			"pxor %%mm0, %%mm0		\n\t"
			"cmpl $0, %5			\n\t"
			"je 1f				\n\t"
			".p2align 4,,7			\n\t"
			"0:				\n\t"

			_txor(0) _txor(4) _txor(8) _txor(12)

			"addl $16, %4			\n\t"
			"cmpl %5, %4			\n\t"
			"jne 0b				\n\t"
			"pxor %%mm0, %0			\n\t"
			"1:				\n\t"

			:"=y"(accum), "=r"(i)
			:"r"(curr_col), "r"(entries), "1"(i), 
				"g"(count & (uint32)(~15))
			:"%eax", "%ecx", "%edx", "%mm0");

		#undef _txor
#else
		accum = 0;
		for (i = 0; i < (count & (uint32)(~15)); i += 16) {
			accum ^= curr_col[entries[i+2+0]] ^
			         curr_col[entries[i+2+1]] ^
			         curr_col[entries[i+2+2]] ^
			         curr_col[entries[i+2+3]] ^
			         curr_col[entries[i+2+4]] ^
			         curr_col[entries[i+2+5]] ^
			         curr_col[entries[i+2+6]] ^
			         curr_col[entries[i+2+7]] ^
			         curr_col[entries[i+2+8]] ^
			         curr_col[entries[i+2+9]] ^
			         curr_col[entries[i+2+10]] ^
			         curr_col[entries[i+2+11]] ^
			         curr_col[entries[i+2+12]] ^
			         curr_col[entries[i+2+13]] ^
			         curr_col[entries[i+2+14]] ^
			         curr_col[entries[i+2+15]];
		}
#endif
		for (; i < count; i++)
			accum ^= curr_col[entries[i+2]];
		curr_b[row] ^= accum;
		entries += count + 2;
	}
}

/*-------------------------------------------------------------------*/
static void mul_one_block(packed_block_t *curr_block,
			uint64 *curr_col, uint64 *curr_b) {

	uint32 i; 
	uint32 j = 0;
	uint32 k;
	uint32 num_entries = curr_block->num_entries;
	entry_idx_t *entries = curr_block->entries;

	/* unroll by 16, i.e. the number of matrix elements
	   in one cache line (usually). For 32-bit x86, we get
	   a huge performance boost by using either SSE or MMX
	   registers; not because they intrinsically are faster,
	   but because using them cuts the number of memory
	   operations in half, allowing the processor to buffer
	   more xor operations. Also replace two 16-bit loads
	   with a single 32-bit load and extra arithmetic to
	   unpack the array indices */

	for (i = 0; i < (num_entries & (uint32)(~15)); i += 16) {
		#ifdef MANUAL_PREFETCH
		PREFETCH(entries + i + 48);
		#endif

#if defined(__GNUC__) && defined(__i386__) && defined(HAS_MMX)
	#define _txor(x)				\
		"movl 4*" #x "(%1,%4,4), %%eax  \n\t"	\
		"movzwl %%ax, %0                \n\t"	\
		"movq (%2,%0,8), %%mm0          \n\t"	\
		"shrl $16, %%eax                \n\t"	\
		"pxor (%3,%%eax,8), %%mm0       \n\t"	\
		"movq %%mm0, (%2,%0,8)          \n\t"

	asm volatile (
		_txor( 0) _txor( 1) _txor( 2) _txor( 3)
		_txor( 4) _txor( 5) _txor( 6) _txor( 7)
		_txor( 8) _txor( 9) _txor(10) _txor(11)
		_txor(12) _txor(13) _txor(14) _txor(15)
		:"=r"(j)
		:"r"(entries), "r"(curr_b), "r"(curr_col),
		 "r"(i), "0"(j)
		:"%mm0", "%eax", "memory");

#elif defined(_MSC_VER) && !defined(_WIN64) && defined(HAS_MMX)
	#define _txor(x)				\
		__asm movzx	eax,word ptr[4*x+2+edx]	\
		__asm movq	mm0,[ecx+eax*8]		\
		__asm movzx	eax,word ptr[4*x+edx]	\
		__asm pxor	mm0,[esi+eax*8]		\
		__asm movq	[esi+eax*8],mm0

	__asm
	{	
		mov		esi,curr_b
		mov		ecx,curr_col
		mov		edx,entries
		mov		eax,i
		lea		edx,[edx+eax*4]
		_txor( 0); _txor( 1); _txor( 2); _txor( 3);
		_txor( 4); _txor( 5); _txor( 6); _txor( 7);
		_txor( 8); _txor( 9); _txor(10); _txor(11);
		_txor(12); _txor(13); _txor(14); _txor(15);
	}
#else
#define _txor(x) curr_b[entries[i+x].row_off] ^= \
				 curr_col[entries[i+x].col_off]
	_txor( 0); _txor( 1); _txor( 2); _txor( 3);
	_txor( 4); _txor( 5); _txor( 6); _txor( 7);
	_txor( 8); _txor( 9); _txor(10); _txor(11);
	_txor(12); _txor(13); _txor(14); _txor(15);
#endif
#undef _txor
	}

	for (; i < num_entries; i++) {
		j = entries[i].row_off;
		k = entries[i].col_off;
		curr_b[j] ^= curr_col[k];
	}
}

/*-------------------------------------------------------------------*/
#ifdef WIN32
static DWORD WINAPI mul_packed_core(LPVOID thread_data) {
#else
static void *mul_packed_core(void *thread_data) {
#endif

	thread_data_t *t = (thread_data_t *)thread_data;
	uint64 *x = t->x;
	uint64 *b = t->b;
	uint32 i;

	/* proceed block by block. We assume that blocks access
	   the matrix in row-major order; when computing b = A*x
	   this will write to the same block of b repeatedly, and
	   will read from all of x. This reduces the number of
	   dirty cache writebacks, improving performance slightly */

	for (i = 0; i < t->num_blocks; i++) {
		packed_block_t *curr_block = t->blocks + i;
		if (curr_block->med_entries)
			mul_one_med_block(curr_block, 
					x + curr_block->start_col,
					b + curr_block->start_row);
		else
			mul_one_block(curr_block, 
					x + curr_block->start_col,
					b + curr_block->start_row);
	}

#ifdef WIN32
	return 0;
#else
	return NULL;
#endif
}

/*-------------------------------------------------------------------*/
static void mul_packed(packed_matrix_t *matrix,
		  uint64 *x, uint64 *b) {

	uint32 i;
	uint32 ncols = matrix->ncols;
	uint32 dense_row_blocks = (matrix->num_dense_rows + 63) / 64;
	thread_id_t thread_id[MAX_THREADS];

	for (i = 0; i < matrix->num_threads; i++) {
		thread_data_t *t = matrix->thread_data + i;

		/* use each thread's scratch vector, except the
		   first thead, which has no scratch vector but
		   uses b instead */

		t->x = x;
		if (i == 0)
			t->b = b;
		memset(t->b, 0, ncols * sizeof(uint64));

		/* fire off each part of the matrix multiply
		   in a separate thread, except the last part.
		   The current thread does the last partial
		   multiply, and this saves one join operation
		   for a noticeable speedup */

		if (i == matrix->num_threads - 1) {
			mul_packed_core(t);
		}
		else {
#ifdef WIN32
			thread_id[i] = CreateThread(NULL, 0, 
						mul_packed_core,
						t, 0, NULL);
#else
			/*
			pthread_create(thread_id + i, NULL, 
						mul_packed_core, t);
						*/
			mul_packed_core( t );
#endif
		}
	}

	/* wait for each thread to finish. All the scratch
	   vectors used by threads get xor-ed into the final b
	   vector */

	for (i = 0; i < matrix->num_threads; i++) {
		if (i < matrix->num_threads - 1) {
#ifdef WIN32
			WaitForSingleObject(thread_id[i], INFINITE);
#else
			// pthread_join(thread_id[i], NULL);
#endif
		}

		if (i > 0) {
			uint64 *curr_b = matrix->thread_data[i].b;
			uint32 j;

			for (j = 0; j < ncols; j++)
				b[j] ^= curr_b[j];
		}
	}

	/* multiply the densest few rows by x (in batches of 64 rows) */

	for (i = 0; i < dense_row_blocks; i++) {
		mul_64xN_Nx64(matrix->dense_blocks[i], 
				x, b + 64 * i, ncols);
	}

#if defined(__GNUC__) && defined(__i386__) && defined(HAS_MMX)
	asm volatile ("emms");
#elif defined(_MSC_VER) && !defined(_WIN64) && defined(HAS_MMX)
	__asm emms
#endif
}

/*-------------------------------------------------------------------*/
static void mul_trans_one_med_block(packed_block_t *curr_block,
			uint64 *curr_row, uint64 *curr_b) {

	uint16 *entries = curr_block->med_entries;

	while (1) {
		uint32 i = 0;
		uint32 row = entries[0];
		uint32 count = entries[1];
		uint64 t;

		if (count == 0)
			break;

		t = curr_row[row];

		/* Unlike the sparse blocks, medium-dense blocks
		   have enough entries that they can be stored in
		   row-major order, with many entries in each row.
		   One iteration of the while loop handles an entire
		   row at a time */

		/* curr_row and curr_b are both cached, so we have to
		   minimize the number of memory accesses and calculate
		   pointers as early as possible */

#if defined(__GNUC__) && defined(__i386__) && defined(HAS_MMX)
		#define _txor(k)				\
			"movl 2*(2+2+(" #k "))(%2,%3,2), %%ecx	\n\t"	\
			"movzwl %%ax, %%edx      	\n\t"	\
			"shrl $16, %%eax         	\n\t"	\
			"movq (%1,%%edx,8), %%mm0	\n\t"	\
			"movq (%1,%%eax,8), %%mm1	\n\t"	\
			"pxor %5, %%mm0			\n\t"	\
			"pxor %5, %%mm1			\n\t"	\
			"movq %%mm0, (%1,%%edx,8)	\n\t"	\
			"movq %%mm1, (%1,%%eax,8)	\n\t"	\
			"movl 2*(2+4+(" #k "))(%2,%3,2), %%eax	\n\t"	\
			"movzwl %%cx, %%edx      	\n\t"	\
			"shrl $16, %%ecx         	\n\t"	\
			"movq (%1,%%edx,8), %%mm0	\n\t"	\
			"movq (%1,%%ecx,8), %%mm1	\n\t"	\
			"pxor %5, %%mm0			\n\t"	\
			"pxor %5, %%mm1			\n\t"	\
			"movq %%mm0, (%1,%%edx,8)	\n\t"	\
			"movq %%mm1, (%1,%%ecx,8)	\n\t"

		asm volatile(
			"movl 2*(2+0)(%2,%3,2), %%eax	\n\t"
			"cmpl $0, %4			\n\t"
			"je 1f				\n\t"
			".p2align 4,,7			\n\t"
			"0:				\n\t"

			_txor(0) _txor(4) _txor(8) _txor(12)

			"addl $16, %3			\n\t"
			"cmpl %4, %3			\n\t"
			"jne 0b				\n\t"
			"1:				\n\t"
			:"=r"(i)
			:"r"(curr_b), "r"(entries), "0"(i), 
				"g"(count & (uint32)(~15)), "y"(t)
			:"%eax", "%ecx", "%edx", "%mm0", "%mm1", "memory");

		#undef _txor
#else
		for (i = 0; i < (count & (uint32)(~15)); i += 16) {
			curr_b[entries[i+2+ 0]] ^= t;
			curr_b[entries[i+2+ 1]] ^= t;
			curr_b[entries[i+2+ 2]] ^= t;
			curr_b[entries[i+2+ 3]] ^= t;
			curr_b[entries[i+2+ 4]] ^= t;
			curr_b[entries[i+2+ 5]] ^= t;
			curr_b[entries[i+2+ 6]] ^= t;
			curr_b[entries[i+2+ 7]] ^= t;
			curr_b[entries[i+2+ 8]] ^= t;
			curr_b[entries[i+2+ 9]] ^= t;
			curr_b[entries[i+2+10]] ^= t;
			curr_b[entries[i+2+11]] ^= t;
			curr_b[entries[i+2+12]] ^= t;
			curr_b[entries[i+2+13]] ^= t;
			curr_b[entries[i+2+14]] ^= t;
			curr_b[entries[i+2+15]] ^= t;
		}
#endif
		for (; i < count; i++)
			curr_b[entries[i+2]] ^= t;
		entries += count + 2;
	}
}

/*-------------------------------------------------------------------*/
static void mul_trans_one_block(packed_block_t *curr_block,
				uint64 *curr_row, uint64 *curr_b) {

	uint32 i;
	uint32 j = 0;
	uint32 k;
	uint32 num_entries = curr_block->num_entries;
	entry_idx_t *entries = curr_block->entries;

	/* unroll by 16, i.e. the number of matrix elements
	   in one cache line (usually). For 32-bit x86, we get
	   a huge performance boost by using either SSE or MMX
	   registers; not because they intrinsically are faster,
	   but because using them cuts the number of memory
	   operations in half, allowing the processor to buffer
	   more xor operations. Also convert two 16-bit reads into
	   a single 32-bit read with unpacking arithmetic */

	for (i = 0; i < (num_entries & (uint32)(~15)); i += 16) {
		#ifdef MANUAL_PREFETCH
		PREFETCH(entries + i + 48);
		#endif
#if defined(__GNUC__) && defined(__i386__) && defined(HAS_MMX)
	#define _txor(x)				\
		"movl 4*" #x "(%1,%4,4), %%eax    \n\t"	\
		"movzwl %%ax, %0                  \n\t"	\
		"movq (%3,%0,8), %%mm0            \n\t"	\
		"shrl $16, %%eax                  \n\t"	\
		"pxor (%2,%%eax,8), %%mm0         \n\t"	\
		"movq %%mm0, (%2,%%eax,8)         \n\t"

	asm volatile (
		_txor( 0) _txor( 1) _txor( 2) _txor( 3)
		_txor( 4) _txor( 5) _txor( 6) _txor( 7)
		_txor( 8) _txor( 9) _txor(10) _txor(11)
		_txor(12) _txor(13) _txor(14) _txor(15)
		:"=r"(j)
		:"r"(entries), "r"(curr_b), "r"(curr_row),
		 "r"(i), "0"(j)
		:"%mm0", "%eax", "memory");

#elif defined(_MSC_VER) && !defined(_WIN64) && defined(HAS_MMX)
	#define _txor(x)				\
		__asm movzx	eax,word ptr[4*x+2+edx]	\
		__asm movq	mm0,[ecx+eax*8]		\
		__asm movzx	eax,word ptr[4*x+edx]	\
		__asm pxor	mm0,[esi+eax*8]		\
		__asm movq	[esi+eax*8],mm0

	__asm
	{	
		mov		esi,curr_b
		mov		ecx,curr_row
		mov		edx,entries
		mov		eax,i
		lea		edx,[edx+eax*4]
		_txor( 0); _txor( 1); _txor( 2); _txor( 3);
		_txor( 4); _txor( 5); _txor( 6); _txor( 7);
		_txor( 8); _txor( 9); _txor(10); _txor(11);
		_txor(12); _txor(13); _txor(14); _txor(15);
	}
#else
#define _txor(x) curr_b[entries[i+x].col_off] ^= \
				 curr_row[entries[i+x].row_off]	
	_txor( 0); _txor( 1); _txor( 2); _txor( 3);
	_txor( 4); _txor( 5); _txor( 6); _txor( 7);
	_txor( 8); _txor( 9); _txor(10); _txor(11);
	_txor(12); _txor(13); _txor(14); _txor(15);
#endif
#undef _txor
	}
	for (; i < num_entries; i++) {
		j = entries[i].row_off;
		k = entries[i].col_off;
		curr_b[k] ^= curr_row[j];
	}
}

/*-------------------------------------------------------------------*/
#ifdef WIN32
static DWORD WINAPI mul_trans_packed_core(LPVOID thread_data) {
#else
static void *mul_trans_packed_core(void *thread_data) {
#endif

	thread_data_t *t = (thread_data_t *)thread_data;
	uint64 *x = t->x;
	uint64 *b = t->b;
	uint32 i;

	/* you would think that doing the matrix multiply
	   in column-major order would be faster, since this would
	   also minimize dirty cache writes. Except that it's slower;
	   it appears that row-major matrix access minimizes some
	   kind of thrashing somewhere */

	for (i = 0; i < t->num_blocks; i++) {
		packed_block_t *curr_block = t->blocks + i;
		if (curr_block->med_entries)
			mul_trans_one_med_block(curr_block, 
					x + curr_block->start_row,
					b + curr_block->start_col);
		else
			mul_trans_one_block(curr_block, 
					x + curr_block->start_row,
					b + curr_block->start_col);
	}

#ifdef WIN32
	return 0;
#else
	return NULL;
#endif
}

/*-------------------------------------------------------------------*/
static void mul_trans_packed(packed_matrix_t *matrix,
			uint64 *x, uint64 *b) {

	uint32 i;
	uint32 ncols = matrix->ncols;
	uint32 dense_row_blocks = (matrix->num_dense_rows + 63) / 64;
	thread_id_t thread_id[MAX_THREADS];
	uint64 *tmp_b[MAX_THREADS];

	memset(b, 0, ncols * sizeof(uint64));

	for (i = 0; i < matrix->num_threads; i++) {
		thread_data_t *t = matrix->thread_data + i;

		/* separate threads fill up disjoint portions
		   of a single b vector, and do not need 
		   per-thread scratch space */

		tmp_b[i] = t->b;
		t->x = x;
		t->b = b;

		/* fire off each part of the matrix multiply
		   in a separate thread, except the last part.
		   The current thread does the last partial
		   multiply, and this saves one join operation
		   for a noticeable speedup */

		if (i == matrix->num_threads - 1) {
			mul_trans_packed_core(t);
		}
		else {
#ifdef WIN32
			thread_id[i] = CreateThread(NULL, 0, 
					mul_trans_packed_core,
					t, 0, NULL);
#else
			/*
			pthread_create(thread_id + i, NULL, 
					mul_trans_packed_core, t);
					*/
			mul_trans_packed_core( t );
#endif
		}
	}

	/* wait for each thread to finish */

	for (i = 0; i < matrix->num_threads; i++) {
		if (i < matrix->num_threads - 1) {
#ifdef WIN32
			WaitForSingleObject(thread_id[i], INFINITE);
#else
			// pthread_join(thread_id[i], NULL);
#endif
		}
		matrix->thread_data[i].b = tmp_b[i];
	}

	/* multiply the densest few rows by x (in batches of 64 rows) */

	for (i = 0; i < dense_row_blocks; i++) {
		mul_Nx64_64x64_acc(matrix->dense_blocks[i], 
				   x + 64 * i, b, ncols);
	}

#if defined(__GNUC__) && defined(__i386__) && defined(HAS_MMX)
	asm volatile ("emms");
#elif defined(_MSC_VER) && !defined(_WIN64) && defined(HAS_MMX)
	__asm emms
#endif
}

/*-------------------------------------------------------------------*/
int compare_row_off(const void *x, const void *y) {
	entry_idx_t *xx = (entry_idx_t *)x;
	entry_idx_t *yy = (entry_idx_t *)y;

	if (xx->row_off > yy->row_off)
		return 1;
	if (xx->row_off < yy->row_off)
		return -1;

	return (int)xx->col_off - (int)yy->col_off;
}

static void matrix_thread_init(thread_data_t *t, la_col_t *A,
			uint32 nrows, uint32 col_min, 
			uint32 col_max, uint32 block_size) {

	uint32 i, j, k, m;
	uint32 num_row_blocks;
	uint32 num_col_blocks;
	packed_block_t *curr_stripe;
	entry_idx_t *e;

	/* allocate blocks in row-major order; a 'stripe' is
	   a vertical column of blocks */

	num_row_blocks = (nrows - NUM_MEDIUM_ROWS + 
				(block_size-1)) / block_size + 1;
	num_col_blocks = ((col_max - col_min + 1) + 
				(block_size-1)) / block_size;
	t->num_blocks = num_row_blocks * num_col_blocks;
	t->blocks = curr_stripe = (packed_block_t *)calloc(
						(size_t)t->num_blocks,
						sizeof(packed_block_t));

	/* we convert the sparse part of the matrix to packed
	   format one stripe at a time. This limits the worst-
	   case memory use of the packing process */

	for (i = 0; i < num_col_blocks; i++, curr_stripe++) {

		uint32 curr_cols = MIN(block_size, (col_max - col_min + 1) - 
						i * block_size);
		packed_block_t *b;

		/* initialize the blocks in stripe i. The first
		   block has NUM_MEDIUM_ROWS rows, and all the
		   rest have block_size rows */

		for (j = 0, b = curr_stripe; j < num_row_blocks; j++) {

			if (j == 0) {
				b->start_row = 0;
				b->num_rows = NUM_MEDIUM_ROWS;
			}
			else {
				b->start_row = NUM_MEDIUM_ROWS +
						(j - 1) * block_size;
				b->num_rows = block_size;
			}

			b->start_col = col_min + i * block_size;
			b += num_col_blocks;
		}

		/* count the number of nonzero entries in each block */

		for (j = 0; j < curr_cols; j++) {
			la_col_t *c = A + col_min + i * block_size + j;

			for (k = 0, b = curr_stripe; k < c->weight; k++) {
				uint32 index = c->data[k];

				while (index >= b->start_row + b->num_rows)
					b += num_col_blocks;
				b->num_entries_alloc++;
			}
		}

		/* concatenate the nonzero elements of the matrix
		   columns corresponding to this stripe. Note that
		   the ordering of elements within a single block
		   is arbitrary, and it's possible that some non-
		   default ordering can enhance performance. However,
		   I've tried a few different ideas and they're much
		   slower than just storing the nonzeros in the order
		   in which they occur in the unpacked matrix 
		   
		   We technically can combine the previous pass through
		   the columns with this pass, but on some versions of
		   libc the number of reallocations causes an incredible
		   slowdown */

		for (j = 0, b = curr_stripe; j < num_row_blocks; j++) {
			b->entries = (entry_idx_t *)malloc(
						b->num_entries_alloc *
						sizeof(entry_idx_t));
			b += num_col_blocks;
		}

		for (j = 0; j < curr_cols; j++) {
			la_col_t *c = A + col_min + i * block_size + j;

			for (k = 0, b = curr_stripe; k < c->weight; k++) {
				uint32 index = c->data[k];

				while (index >= b->start_row + b->num_rows)
					b += num_col_blocks;

				e = b->entries + b->num_entries++;
				e->row_off = index - b->start_row;
				e->col_off = j;
			}

			free(c->data);
			c->data = NULL;
		}

		/* now convert the first block in the stripe to
		   a somewhat-compressed format. Entries in this
		   first block are stored by row, and all rows
		   are concatenated into a single 16-bit array */

		b = curr_stripe;
		e = b->entries;
		qsort(e, (size_t)b->num_entries, 
				sizeof(entry_idx_t), compare_row_off);
		for (j = k = 0; j < b->num_entries; j++) {
			if (j == 0 || e[j].row_off != e[j-1].row_off)
				k++;
		}

		/* we need a 16-bit word for each element and two more
		   16-bit words at the start of each of the k packed
		   arrays making up med_entries. The first extra word
		   gives the row number and the second gives the number
		   of entries in that row. We also need a few extra words 
		   at the array end because the multiply code uses a 
		   software pipeline and would fetch off the end of 
		   med_entries otherwise */

		b->med_entries = (uint16 *)malloc((b->num_entries + 
						2 * (k + 4)) * sizeof(uint16));
		j = k = 0;
		while (j < b->num_entries) {
			for (m = 0; j + m < b->num_entries; m++) {
				if (m > 0 && 
				    e[j+m].row_off != e[j+m-1].row_off)
					break;
				b->med_entries[k+m+2] = e[j+m].col_off;
			}
			b->med_entries[k] = e[j].row_off;
			b->med_entries[k+1] = m;
			j += m;
			k += m + 2;
		}
		b->med_entries[k] = b->med_entries[k+1] = 0;
		free(b->entries);
		b->entries = NULL;
	}
}

/*-------------------------------------------------------------------*/
void packed_matrix_init(msieve_obj *obj,
			packed_matrix_t *p, la_col_t *A,
			uint32 nrows, uint32 ncols,
			uint32 num_dense_rows) {

	uint32 i, j, k;
	uint32 dense_row_blocks;
	uint32 block_size;
	uint32 num_threads;
	uint32 num_nonzero;
	uint32 num_nonzero_per_thread;

	/* initialize */

	memset(p, 0, sizeof(packed_matrix_t));
	p->unpacked_cols = A;
	p->nrows = nrows;
	p->ncols = ncols;
	p->num_dense_rows = num_dense_rows;

	if (ncols <= MIN_NCOLS_TO_PACK)
		return;

	p->unpacked_cols = NULL;

	/* decide on the block size. We want to split the
	   cache into thirds; one third for x, one third for
	   b and the last third for streaming access to 
	   each block. If the matrix is small, adjust the
	   block size so that the matrix is divided 2.5
	   ways in each dimension.
	  
	   Note that the following does not compensate for
	   having multiple threads. If each thread gets a 
	   separate processor, no compensation is needed.
	   This could conceivably cause problems if multiple 
	   cores share the same cache, but multicore processors 
	   typically have pretty big caches anyway */

	block_size = obj->cache_size2 / (3 * sizeof(uint64));
	block_size = MIN(block_size, ncols / 2.5);
	block_size = MIN(block_size, 65536);
	if (block_size == 0)
		block_size = 32768;

	logprintf(obj, "using block size %u for "
			"processor cache size %u kB\n", 
				block_size, obj->cache_size2 / 1024);

	/* pack the dense rows 64 at a time */

	dense_row_blocks = (num_dense_rows + 63) / 64;
	if (dense_row_blocks) {
		p->dense_blocks = (uint64 **)malloc(dense_row_blocks *
						sizeof(uint64 *));
		for (i = 0; i < dense_row_blocks; i++) {
			p->dense_blocks[i] = (uint64 *)malloc(ncols *
							sizeof(uint64));
		}

		for (i = 0; i < ncols; i++) {
			la_col_t *c = A + i;
			uint32 *src = c->data + c->weight;
			for (j = 0; j < dense_row_blocks; j++) {
				p->dense_blocks[j][i] = 
						(uint64)src[2 * j + 1] << 32 |
						(uint64)src[2 * j];
			}
		}
	}

	/* determine the number of threads to use */

	num_threads = obj->num_threads;
	if (num_threads < 2 || ncols < MIN_NCOLS_TO_THREAD)
		num_threads = 1;
	p->num_threads = num_threads = MIN(num_threads, MAX_THREADS);

	/* compute the number of nonzero elements in the submatrix
	   given to each thread */

	for (i = num_nonzero = 0; i < ncols; i++)
		num_nonzero += A[i].weight;
	num_nonzero_per_thread = num_nonzero / num_threads;

	/* divide the matrix into groups of columns, one group
	   per thread, and pack each group separately */

	for (i = j = k = num_nonzero = 0; i < ncols; i++) {

		num_nonzero += A[i].weight;

		if (i == ncols - 1 || num_nonzero >= num_nonzero_per_thread) {
			matrix_thread_init(p->thread_data + k, A,
						nrows, j, i, block_size);

			/* each thread needs scratch space to store
			   matrix products. The first thread doesn't need
			   scratch space, it's provided by calling code */

			if (k > 0) {
				p->thread_data[k].b = (uint64 *)malloc(ncols * 
							sizeof(uint64));
			}
			j = i + 1;
			k++;
			num_nonzero = 0;
		}
	}
}

/*-------------------------------------------------------------------*/
void packed_matrix_free(packed_matrix_t *p) {

	uint32 i, j;

	if (p->unpacked_cols) {
		la_col_t *A = p->unpacked_cols;
		for (i = 0; i < p->ncols; i++) {
			free(A[i].data);
			A[i].data = NULL;
		}
	}
	else {
		for (i = 0; i < p->num_threads; i++) {
			thread_data_t *t = p->thread_data + i;
			for (j = 0; j < t->num_blocks; j++) {
				free(t->blocks[j].entries);
				free(t->blocks[j].med_entries);
			}
			free(t->blocks);
			if (i > 0) {
				free(t->b);
			}
		}
		for (i = 0; i < (p->num_dense_rows + 63) / 64; i++)
			free(p->dense_blocks[i]);
		free(p->dense_blocks);
	}
}

/*-------------------------------------------------------------------*/
void mul_MxN_Nx64(packed_matrix_t *A, uint64 *x, uint64 *b) {

	/* Multiply the vector x[] by the matrix A (stored
	   columnwise) and put the result in b[]. */

	if (A->unpacked_cols)
		mul_unpacked(A, x, b);
	else
		mul_packed(A, x, b);
}

/*-------------------------------------------------------------------*/
void mul_trans_MxN_Nx64(packed_matrix_t *A, uint64 *x, uint64 *b) {

	/* Multiply the vector x[] by the transpose of the
	   matrix A and put the result in b[]. Since A is stored
	   by columns, this is just a matrix-vector product */

	if (A->unpacked_cols)
		mul_trans_unpacked(A, x, b);
	else
		mul_trans_packed(A, x, b);
}
