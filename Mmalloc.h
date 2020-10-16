#ifndef _MALLOC_H
#define _MALLOC_H 

#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <mcheck.h>
#include <utility>

namespace global{

const int ListSize = 7;
const int InitNum = 3;
struct block_info;

//
static pthread_mutex_t global_heap_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t global_Bin_a = PTHREAD_MUTEX_INITIALIZER;

void* align8(void *x);
void* heap_allocate(size_t size);
void* find_best_fit_from_bin_large(size_t size);
void* mmap_new_memory(size_t size);
void* allocLarge(size_t);
void* allocLargeByMmap(size_t);
void* findBestFitFromLarge(size_t);

// Operation About Bin_a 
void* findBestBlockFromBinA(size_t size);
bool addBlockToBinA(void*);
bool deleteBlockFromBinA(void*);


block_info** InitListHelper(size_t size);
// FIXME : 初始化有问题
size_t InitGlobalList(void* ptr);

namespace tcmalloc{

class threadCache{
public:
   static pthread_key_t heapKey;

   static threadCache* getCache();
   static threadCache* createCache();
   static void destroyThreadCache(void* ptr);
   static void deleteCache(threadCache*);
   static bool hasInit();

   void cleanUp();

   void Init(pthread_t tid);
   static void InitTSD();

   int getTheListIndex(size_t size);
   void* allocate(size_t size);
   void deallocate(void* ptr);
   block_info* freeList[ListSize];

private:
   std::pair<size_t, block_info*> FromInitList(size_t);
   block_info* FromGlobalList(size_t);
   void* FromHeap(size_t);

   
   static threadCache* mThreadCache;
   static bool isInit;
   size_t size_;

public:
   threadCache* prev;
   threadCache* next;
   pthread_t tid_;

};


void* Mmalloc(size_t);
void Mfree(void*);


} // namespace tcmalloc

} // namespace global

#endif
