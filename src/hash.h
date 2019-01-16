#ifndef __EACSMB_HASH_H__
#define __EACSMB_HASH_H__

#include <stdint.h>


struct hash_bucket {
	uint64_t hash;
	char* key;
	void* value;
};


typedef struct hash_table {
	size_t alloc_size;
	size_t fill;
	float grow_ratio; // default 0.75
	float shrink_ratio; // set greater than 1.0 to entirely disable, default 99.0
	struct hash_bucket* buckets; 
} HashTable;

#define HashTable(x) struct hash_table

// NOTE: if you pass in garbage pointers you deserve the segfault

HashTable* HT_create(int allocPOT);
int HT_init(HashTable* obj, int allocPOT);
void HT_destroy(HashTable* obj, int free_values_too);
int HT_resize(HashTable* obj, int newSize);

// returns 0 if val is set to the value
// *val == NULL && return > 0  means the key was not found;
int HT_get(HashTable* obj, char* key, void** val);
int HT_getInt(HashTable* obj, char* key, int64_t* val);

// zero for success
// key's memory is not managed internally. strdup it yourself
int HT_set(HashTable* obj, char* key, void* val);
int HT_setInt(HashTable* obj, char* key, int64_t val);

int HT_delete(HashTable* obj, char* key);

// iteration. no order. results undefined if modified while iterating
// returns 0 when there is none left
// set iter to NULL to start
int HT_next(HashTable* obj, void** iter, char** key, void** value);

/*
Loop macro magic

https://www.chiark.greenend.org.uk/~sgtatham/mp/

HashTable obj;
HT_LOOP(&obj, key, char*, val) {
	printf("loop: %s, %s", key, val);
}

effective source:

	#define HT_LOOP(obj, keyname, valtype, valname)
	if(0)
		finished: ;
	else
		for(char* keyname;;) // internal declarations, multiple loops to avoid comma op funny business
		for(valtype valname;;)
		for(void* iter = NULL ;;)
			if(HT_next(obj, iter, &keyname, &valname))
				goto main_loop;
			else
				while(1)
					if(1) {
						// when the user uses break
						goto finished;
					}
					else
						while(1)
							if(!HT_next(obj, iter, &keyname, &valname)) {
								// normal termination
								goto finished;
							}
							else
								main_loop:
								//	{ user block; not in macro }
*/
#define HASH__PASTEINNER(a, b) a ## b
#define HASH__PASTE(a, b) HASH__PASTEINNER(a, b) 
#define HASH__ITER(key, val) HASH__PASTE(hashtable_iter_ ## key ## __ ## val ## __, __LINE__)
#define HASH__FINISHED(key, val) HASH__PASTE(hashtable_finished__ ## key ## __ ## val ## __, __LINE__)
#define HASH__MAINLOOP(key, val) HASH__PASTE(hashtable_main_loop__ ## key ## __ ## val ## __, __LINE__)    
#define HT_LOOP(obj, keyname, valtype, valname) \
if(0) \
	HASH__FINISHED(key, val): ; \
else \
	for(char* keyname ;;) \
	for(valtype valname ;;) \
	for(void* HASH__ITER(key, val) = NULL ;;) \
		if(HT_next(obj, & (HASH__ITER(key, val)), &keyname, (void**)&valname)) \
			goto HASH__MAINLOOP(key, val); \
		else \
			while(1) \
				if(1) { \
					goto HASH__FINISHED(key, val); \
				} \
				else \
					while(1) \
						if(!HT_next(obj, & (HASH__ITER(key, val)), &keyname, (void**)&valname)) { \
							goto HASH__FINISHED(key, val); \
						} \
						else \
							HASH__MAINLOOP(key, val) :
							
							//	{ user block; not in macro }




// special faster version for storing just integer sets
struct int_hash_bucket {
	uint64_t key;
	uint64_t value;
};

typedef struct int_hash_table {
	size_t alloc_size;
	size_t fill;
	float grow_ratio;
	float shrink_ratio;
	struct int_hash_bucket* buckets; 
} IntHash;



#define HT_TYPE(x) _Generic( (x), \
	default: 'std', \
	struct hash_table: HT_STD, \
	struct int_hash_table: HT_INT, \
)









#endif //__EACSMB_HASH_H__
