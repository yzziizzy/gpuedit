#ifndef __EACSMB_mempool_h__
#define __EACSMB_mempool_h__

#include <stdlib.h>
#include <stdint.h>

// MemPool is a FAST, non-thread-safe allocator for fixed-size objects.
//  There are no system calls during allocation or freeing, though the kernel may run
//  interrupts when a new page of virtual memory is written to for the first time.

// Does not:
//   Handle double free()'s. Only call free once.
//   Handle threads. Manage synchronization yourself.
//   Grow the pool dynamically. Request enough space from the start.
//   Reserve physical memory with the OS. It's purely virtual until you use it.
//   Check if the pointer you feed to free belongs to this pool. Be careful.



typedef struct MemPool {
	
	size_t itemSize;
	size_t maxItems;
	
	size_t fill;
	size_t highestUsed;
	
	size_t firstFree;
	
	void* pool;
	
	
} MemPool;

// DO NOT USE THIS:
// these allocate the pool itself
MemPool* MemPool_alloc(size_t itemSize, size_t maxItems);
void MemPool_init(MemPool* mp, size_t itemSize, size_t maxItems);

// USE THIS:
// these allocate chunks of memory from the pool
void* MemPool_malloc(MemPool* mp);
void MemPool_free(MemPool* mp, void* ptr);




// -----------------------------------------------------------------




//
// Tracked MemPool includes a bitfield tracking usage, making it iterable and double-free safe
//

// Does:
//   Handle double free()'s. Call free as much as you want.

// Does not:
//   Handle threads. Manage synchronization yourself.
//   Grow the pool dynamically. Request enough space from the start.
//   Reserve physical memory with the OS. It's purely virtual until you use it.
//   Check if the pointer you feed to free belongs to this pool. Be careful.



typedef struct MemPoolT /* racked */ {
	
	size_t itemSize;
	size_t maxItems;
	
	size_t fill;
	size_t highestUsed;
	
	size_t firstFree;
	
	size_t bitfieldAlloc;
	uint64_t* bitfield; 
	void* pool;
	
} MemPoolT;




// DO NOT USE THIS:
// these allocate the pool itself
MemPoolT* MemPoolT_alloc(size_t itemSize, size_t maxItems);
void MemPoolT_init(MemPoolT* mp, size_t itemSize, size_t maxItems);

// USE THIS:
// these allocate chunks of memory from the pool
void* MemPoolT_malloc(MemPoolT* mp);
void MemPoolT_free(MemPoolT* mp, void* ptr);


// these are 0-based indices used for iteration
int MemPoolT_isSlotUsed(MemPoolT* mp, size_t index);
void* MemPoolT_getNextUsedIndex(MemPoolT* mp, size_t* index);
int MemPoolT_ownsPointer(MemPoolT* mp, void* ptr);

static inline size_t MemPoolT_maxIndex(MemPoolT* mp) { 
	return mp->highestUsed == 0 ? 0 : mp->highestUsed - 1;
}

static inline void* MemPoolT_getIndex(MemPoolT* mp, size_t index) { 
	return mp->pool + (index * mp->itemSize);
}



// VECMP is a set of typesafe vector macros built around a tracked mempool
// being backed by a mempool, pointers to items are stable forever

#define VECMP(t) \
struct { \
	t* lastInsert; \
	MemPoolT pool; \
}



// initialisze a mempool vector
#define VECMP_INIT(x, maxItems) \
do { \
	(x)->lastInsert = NULL; \
	MemPoolT_init(&(x)->pool, sizeof(*(x)->lastInsert), maxItems); \
} while(0)


#define VECMP_INSERT(x, e) \
do { \
	(x)->lastInsert = MemPoolT_malloc(&(x)->pool); \
	*((x)->lastInsert) = (e); \
} while(0)


#define VECMP_INC(x) \
do { \
	(x)->lastInsert = MemPoolT_malloc(&(x)->pool); \
} while(0)


#define VECMP_LAST_INSERT(x) ((x)->lastInsert)


#define VECMP_DELETE(x, ptr) \
do { \
	MemPoolT_free(&(x)->pool, ptr); \
} while(0)


#define VECMP_LEN(x) ((x)->pool.fill) 
#define VECMP_OWNS_PTR(x, ptr) (MemPoolT_ownsPointer(&(x)->pool, ptr)) 




#define VECMP__PASTEINNER(a, b) a ## b
#define VECMP__PASTE(a, b) VECMP__PASTEINNER(a, b) 
#define VECMP__ITER(key, val) VECMP__PASTE(VEC_iter_ ## key ## __ ## val ## __, __LINE__)
#define VECMP__FINISHED(key, val) VECMP__PASTE(VECMP_finished__ ## key ## __ ## val ## __, __LINE__)
#define VECMP__MAINLOOP(key, val) VECMP__PASTE(VECMP_main_loop__ ## key ## __ ## val ## __, __LINE__)    
#define VECMP_EACH(obj, index, valname) \
if(0) \
	VECMP__FINISHED(index, val): ; \
else \
	for(typeof((obj)->lastInsert) valname ;;) \
	for(size_t index = 0;;) \
		if(index < MemPoolT_maxIndex(&(obj)->pool) && (valname = MemPoolT_getNextUsedIndex(&(obj)->pool, &index), 1)) \
			goto VECMP__MAINLOOP(index, val); \
		else \
			while(1) \
				if(1) { \
					goto VECMP__FINISHED(index, val); \
				} \
				else \
					while(1) \
						if(++index >= MemPoolT_maxIndex(&(obj)->pool) || (valname = MemPoolT_getNextUsedIndex(&(obj)->pool, &index), 0)) { \
							goto VECMP__FINISHED(index, val); \
						} \
						else \
							VECMP__MAINLOOP(index, val) :
							
							//	{ user block; not in macro }







#endif //__EACSMB_mempool_h__
