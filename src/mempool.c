#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <sys/mman.h>

#include "mempool.h"









MemPool* MemPool_alloc(size_t itemSize, size_t maxItems) {
	MemPool* mp;
	
	mp = calloc(1, sizeof(*mp));
	
	MemPool_init(mp, itemSize, maxItems);
	
	return mp;
}



void MemPool_init(MemPool* mp, size_t itemSize, size_t maxItems) {
	size_t allocSize;
	
	mp->itemSize = itemSize < sizeof(size_t) ? sizeof(size_t) : itemSize;
	mp->maxItems = maxItems;
	
	mp->fill = 0;
	mp->firstFree = 1; // first free is 1-based
	
	allocSize = mp->itemSize * mp->maxItems;
	int flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE;
	mp->pool = mmap(NULL, allocSize, PROT_READ | PROT_WRITE, flags, 0, 0);
	if(mp->pool == MAP_FAILED) {
		fprintf(stderr, "mmap failed in %s: %s\n", __func__, strerror(errno));
		exit(1);
	}
}



void* MemPool_malloc(MemPool* mp) {
	if(mp->fill >= mp->maxItems) {
		fprintf(stderr, "MemPool overflowed max items %d\n", mp->maxItems);
		return NULL;
	}
	
	size_t off = mp->itemSize * (mp->firstFree - 1);
	size_t next = *(size_t*)(mp->pool + off);
	if(next == 0) next = mp->firstFree + 1;
	mp->firstFree = next;
	
	// not used in operation
	mp->fill++;
	
	return mp->pool + off;
}


void MemPool_free(MemPool* mp, void* ptr) {
	
	size_t ooff = mp->itemSize * (mp->firstFree - 1);
	size_t noff = (ptr - mp->pool) / mp->itemSize;
	
	*(size_t*)(mp->pool + ooff) = noff + 1;
	
	// not used in operation
	mp->fill--;
}



// -------------------------------------------
// tracked mempool



MemPoolT* MemPoolT_alloc(size_t itemSize, size_t maxItems) {
	MemPoolT* mp;
	
	mp = calloc(1, sizeof(*mp));
	
	MemPoolT_init(mp, itemSize, maxItems);
	
	return mp;
}



void MemPoolT_init(MemPoolT* mp, size_t itemSize, size_t maxItems) {
	size_t allocSize;
	
	mp->itemSize = itemSize < sizeof(size_t) ? sizeof(size_t) : itemSize;
	mp->maxItems = maxItems;
	
	mp->fill = 0;
	mp->firstFree = 1; // first free is 1-based
	mp->highestUsed = 0;
	
	mp->bitfieldAlloc = 0;
	mp->bitfield = NULL;
	
	allocSize = mp->itemSize * mp->maxItems;
	int flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE;
	mp->pool = mmap(NULL, allocSize, PROT_READ | PROT_WRITE, flags, 0, 0);
	if(!mp->pool || mp->pool == MAP_FAILED) {
		fprintf(stderr, "mmap failed in %s: %s\n", __func__, strerror(errno));
		exit(1);
	}
}





static inline void mpt_check_bitfield(MemPoolT* mp) {
	printf("bf alloc: %d\n", mp->bitfieldAlloc);
	if((mp->fill / 64) + ((mp->fill % 64) > 0) >= mp->bitfieldAlloc) {
		mp->bitfieldAlloc = mp->bitfieldAlloc < 8 ? 8 : mp->bitfieldAlloc * 2;
		mp->bitfield = realloc(mp->bitfield, sizeof(*mp->bitfield) * mp->bitfieldAlloc);
	}
}

static inline size_t mpt_get_bf_index(MemPoolT* mp, size_t index) {
	return ((index - 1) / 64);
}

static inline int mpt_get_bf_bit(MemPoolT* mp, size_t index) {
	return ((index - 1) % 64);
}

static inline uint64_t mpt_get_bf_mask(MemPoolT* mp, size_t index) {
	return ((uint64_t)1) << mpt_get_bf_bit(mp, index);
}

static inline void mpt_set_bit(MemPoolT* mp, size_t index) {
	uint64_t mask = mpt_get_bf_mask(mp, index);
	int i = mpt_get_bf_index(mp, index);
	mp->bitfield[i] |= mask;
}
static inline void mpt_clear_bit(MemPoolT* mp, size_t index) {
	mp->bitfield[mpt_get_bf_index(mp, index)] &= ~mpt_get_bf_mask(mp, index);
}
static inline int mpt_get_bit(MemPoolT* mp, size_t index) {
	return 0 != (mp->bitfield[mpt_get_bf_index(mp, index)] & mpt_get_bf_mask(mp, index));
}





void* MemPoolT_malloc(MemPoolT* mp) {
	if(mp->fill >= mp->maxItems) {
		fprintf(stderr, "MemPool overflowed max items %d\n", mp->maxItems);
		return NULL;
	}
	
	mpt_check_bitfield(mp);
	
	size_t off = mp->itemSize * (mp->firstFree - 1);
	size_t next = *(size_t*)(mp->pool + off);
	if(next == 0) next = mp->firstFree + 1;
	
	mpt_set_bit(mp, mp->firstFree);
	
	mp->firstFree = next;
	mp->highestUsed = next > mp->highestUsed ? next : mp->highestUsed;
	mp->fill++;
	
	return mp->pool + off;
}


void MemPoolT_free(MemPoolT* mp, void* ptr) {
	
	size_t ooff = mp->itemSize * (mp->firstFree - 1);
	size_t noff = (ptr - mp->pool) / mp->itemSize;
	
	if(mpt_get_bit(mp, noff + 1)) {
		mpt_clear_bit(mp, noff + 1);
		
		*(size_t*)(mp->pool + ooff) = noff + 1;
		
		mp->fill--;
	}
}



// these are 0-based indices used for iteration
int MemPoolT_isSlotUsed(MemPoolT* mp, size_t index) {
	return mpt_get_bit(mp, index + 1);
}

void* MemPoolT_getNextUsedIndex(MemPoolT* mp, size_t* index) { 
	if(mp->fill <= 0) return NULL;
	
	while(!mpt_get_bit(mp, *index + 1)) {
		*index++;
		
		if(*index >= mp->highestUsed - 1) {
			return NULL;
		}
	}

	if(*index >= mp->highestUsed - 1) return NULL;
	
	return mp->pool + (*index * mp->itemSize);
}


int MemPoolT_ownsPointer(MemPoolT* mp, void* ptr) { 
	return ptr >= mp->pool && ptr < mp->pool + (mp->itemSize * mp->maxItems);
}
