#include "Mmalloc.h"
#include <assert.h>
#include <iostream>
#include <utility>
#include <functional>
#include <map>
#include <unordered_map>


using namespace global;
// ..
namespace global{


typedef struct block_info
{
   int size;
   struct block_info *next;
   
}block_info;
// static Init

bool tcmalloc::threadCache::isInit = false;
pthread_key_t tcmalloc::threadCache::heapKey;
tcmalloc::threadCache* tcmalloc::threadCache::mThreadCache = nullptr;
bool tcmalloc::threadCache::hasInit(){
   return isInit;
}
//static Init end;

// bin_a仅用于小内存块的空余内存块存储
// bin_large用于大块内存的分配
  static block_info *bin_a     = nullptr;
__thread block_info *bin_large = nullptr;

thread_local std::unordered_map<size_t, block_info*> bins;

void *heap_used_memory_end = nullptr;
__thread bool hasMapInit = false;
__thread void *thread_unused_heap_start = nullptr;
__thread void *thread_heap_end = nullptr;


block_info* InitMapHelper(size_t size){
    return bins[size];
}

// TODO 
size_t InitMap(void* ptr){
    size_t allSize = 0;
    size_t curSize = 8;

    if(!hasMapInit){

        size_t cur_ = 8;
        for(int idx = 0; idx < ListSize; idx++){
            // bins.insert(std::make_pair(cur_, nullptr));
            bins[cur_] = nullptr;
            cur_ <<= 1;
        }
        hasMapInit = true;

    }

    for(int idx = 0; idx < ListSize; idx++){
        block_info* cur = InitMapHelper(curSize);
        size_t offset = sizeof(block_info) + curSize;

        for(int num = 0; num < InitNum; num++){
            if (cur == nullptr)
            {
                cur = (block_info*)(ptr + allSize);
                bins[curSize] = cur;
            }
            cur -> size = curSize;
            // size_t offset = sizeof(block_info) + curSize;
            cur -> next = num == (InitNum-1) ? nullptr : (block_info*)(ptr + (allSize + offset));
            allSize += offset;
            cur = (block_info*)(ptr + allSize);
        }
        curSize <<= 1;
    }
    return allSize;
}

/*
  Aligns pointer to 8 byte address.
  params : pointer to align.
  returns: a pointer (aligned to 8 bytes)
 */
void *align8(void *x)
{
    unsigned long p = (unsigned long)x;
    p = (((p - 1) >> 3) << 3) + 8;
    // p = (((p-1) & ~0x7)) + 8;
    return (void *)p;
}


/* 空闲链表中搜索最优的内存 */
void* findBestBlockFromBinA(size_t size)
{
    block_info* ptr = bin_a;
    block_info* bestPtr = nullptr;
    int minFitSize = INT_MAX;
    void* ret = nullptr;
    while(ptr != nullptr){
        if(ptr -> size > size && ptr -> size < minFitSize){
            bestPtr = ptr;
            minFitSize = ptr -> size;
        }
        ptr = ptr -> next;
    }
    if(bestPtr != nullptr){
        
        // 是bin_a的头部内存块
        if(bestPtr == bin_a){
            bin_a = bin_a -> next;
            bestPtr -> next = nullptr;
            ret = reinterpret_cast<void*>(bestPtr);
        }
        // 不需要更新bin_a的头
        else{
            ptr = bin_a;
            while(ptr != nullptr && ptr -> next != bestPtr){
                ptr = ptr -> next;
            }
            if (ptr != nullptr)
            {
                ptr -> next = bestPtr -> next;
            }
            bestPtr -> next = nullptr;
            ret = reinterpret_cast<void*>(bestPtr);
            
        }
    }

    return ret;
}

/*
  Finds best fitting block from large memory bin.
*/
void * find_best_fit_from_bin_large(size_t size)
{
    block_info *b = bin_large;
    block_info *best_fit = NULL;
    int min_fit_size = INT_MAX;
    void *ret = NULL;

    while(b != NULL)
    {
         if(b->size >= size && b->size < min_fit_size)
         {
            best_fit = b;
            min_fit_size = b->size;
         }
         b = b->next;
    }

    /*If best fit found, update list*/
    if(NULL != best_fit)
    {
        // if best_fit is first block.
        if (best_fit == bin_large)
        {
           bin_large = bin_large->next;
           best_fit->next = NULL;
           ret = (void *)((void *)best_fit + sizeof(block_info));
        }
        else
        {
          b = bin_large;
          while(b != NULL && b->next != best_fit)
          {
            b = b->next;
          }
          if(b != NULL)
          {
             b->next = best_fit->next;
          }
          best_fit->next = NULL;
          ret = (void *)((void *)best_fit + sizeof(block_info));
        }
    }

    return ret;
}

void* findBestFitFromLarge(size_t size)
{
    return find_best_fit_from_bin_large(size);
}

namespace tcmalloc
{
    
// 将threadCache里面的链表全部清空,并加入到全局的空闲链表中
// FIXME : 线程结束后调用会出错
void threadCache::cleanUp(){
    for (size_t i = 0; i < ListSize; i++)
    {
        block_info* _list = freeList[i];
        // threadCache中~有已经分配的内存
        if(_list != nullptr){
            // free the list to the heap
            block_info* cur = _list;
            std::cout << cur ->size << std::endl;
            while (cur != nullptr) 
            {
                // SIGSEGV in here
                // block_info* _cur = (block_info*)(cur + sizeof(block_info));
                // memset(_cur, '\0', cur -> size);
                if(cur -> next == nullptr){
                    cur -> next = bin_a;
                    bin_a = _list;
                    break;
                }
                cur = cur -> next;
            }
            
        }
        
    }
    
}

void threadCache::deleteCache(threadCache* heap){
    // TODO: 归还threadCache的内存空间 或者加入到空闲链表(bin_a)中
    heap -> cleanUp();
}

void threadCache::Init(pthread_t tid){
    tid_ = tid;
    // freeList Init here
    for(size_t idx = 0; idx < ListSize; idx++){
        freeList[idx] = nullptr;
    }
}

void threadCache::destroyThreadCache(void* ptr){
    if(ptr != nullptr){
        deleteCache(reinterpret_cast<threadCache*>(ptr));
    }
}

void threadCache::InitTSD() {
  if(isInit) return;
  pthread_key_create(&heapKey, destroyThreadCache);
  isInit = true;
}

threadCache* threadCache::createCache(){
    // threadCache* heap = mThreadCache;
    threadCache* heap = nullptr;
    // allocate heap from system
    {
        pthread_mutex_lock(&global_heap_mutex);
        // get threadCacheHeap from heap
        if(heap_used_memory_end == nullptr){
            heap_used_memory_end = sbrk(0);
            align8(heap_used_memory_end);
            // 400kb
            if(sbrk(sysconf(_SC_PAGESIZE) * 100) == (void *) -1)
            {
                errno = ENOMEM;
                perror("\n sbrk failed to extend heap.");
                return NULL;
            }
            // 预先初始化一些链表以供使用.
            size_t ret = InitMap(heap_used_memory_end);
            heap_used_memory_end = heap_used_memory_end + ret;
            if(heap_used_memory_end == (void*) -1)
            {
                errno = ENOMEM;
                perror("\n sbrk(0) failed.");
                return NULL;
            }
        }
        // threadCache 
        heap = reinterpret_cast<threadCache*>(heap_used_memory_end);
        thread_unused_heap_start = heap_used_memory_end;
        // heap_used_memory_end = heap_used_memory_end + (64 << 10);
        
        // default allocate 4KB 
        thread_heap_end = heap_used_memory_end + (sysconf(_SC_PAGESIZE));
        
        // thread_heap_end = heap_used_memory_end;
        heap_used_memory_end = thread_heap_end;
        heap -> Init(pthread_self());

        pthread_mutex_unlock(&global_heap_mutex);
        // mThreadCache = heap;
    }

    // 设置线程私有的threadCache
    pthread_setspecific(heapKey, heap);
    return heap;
}

threadCache* threadCache::getCache()
{
    return isInit
             ? reinterpret_cast<threadCache*>(pthread_getspecific(heapKey))
             : nullptr;
}

/* 返回对应size的freeList的索引 */
int threadCache::getTheListIndex(size_t size)
{
    assert(size > 0);
    size_t curSize = 8;
    int res = 0;
    while (curSize)
    {
        if(curSize >= size){
            return res;
        }
        res++;
        curSize <<= 1;
    }
    return -1;
}

// 
std::pair<size_t, block_info*> threadCache::FromInitList(size_t size) {
    assert(size <= 512);
    size_t alignSize = 8;
    while(!(alignSize / size)){
        alignSize <<= 1;
    }
    return std::make_pair(alignSize, bins[alignSize]);
}

// 从全局空闲链表申请内存
block_info* threadCache::FromGlobalList(size_t size)
{
    pthread_mutex_lock(&global_Bin_a);
    block_info* ptr = bin_a;
    if(ptr != nullptr){
        void* cur = findBestBlockFromBinA(size);
        ptr = reinterpret_cast<block_info*>(cur);
        // XXX : size大小
        // ptr -> size = size;
    }
    pthread_mutex_unlock(&global_Bin_a);
    return ptr;
}

// 从堆上申请
void* threadCache::FromHeap(size_t size){
    void* ptr = heap_used_memory_end;
    assert(ptr != nullptr);
    if(((char*)thread_heap_end - (char*)thread_unused_heap_start )
    < (sizeof(block_info) + size))
    {
        // FIXME : 
        if ((char*)sbrk(0) - (char*)heap_used_memory_end < sizeof(block_info) + size){
            void* res = sbrk(10 * sysconf(_SC_PAGESIZE));
            if(res == reinterpret_cast<void*>(-1)){
                errno = ENOMEM;
                perror("\n sbrk failed to extend heap.");
                return nullptr;
            }
            align8(heap_used_memory_end);
            
        }
        thread_unused_heap_start = heap_used_memory_end;
        thread_heap_end = heap_used_memory_end + (sysconf(_SC_PAGESIZE));
        heap_used_memory_end =  thread_heap_end;
    }
    block_info b;
    // FIXME: 
    b.size = size;
    b.next = nullptr;
    memcpy(thread_unused_heap_start, &b, sizeof(block_info));
    thread_unused_heap_start = ((void*)thread_unused_heap_start 
    + (sizeof(block_info) + size));

    return  (thread_unused_heap_start - (sizeof(block_info)+size));
}

void* threadCache::allocate(size_t size){
    void* ret = nullptr;
    int index = getTheListIndex(size);
    if(index == -1){
        // errno = ..;
        perror("index out of range!\n");
        return nullptr;
    }
    if(freeList[index] != nullptr){
        block_info* cur = freeList[index];
        cur -> size = size;
        ret = reinterpret_cast<void*>(cur) + sizeof(block_info);
        cur = cur -> next;
        freeList[index] = cur;
    }
    // freeList is nullptr.
    /*
     * 1. 从已经初始化的局部链表申请内存
     * 2. 从全局空闲链表中搜索最合适的内存
     * 3. 从堆中申请内存(通过移动heap_used_memory_end)
     */
    else{
        // TODO FromInitList
        std::pair<size_t, block_info*> curListP = FromInitList(size);
        block_info* curList = curListP.second;
        size_t sz = curListP.first;
        if(curList != nullptr){
            block_info* p = curList;
            curList = p -> next;
            p -> next = nullptr;
            // block_info** binPtr = InitListHelper(p -> size);
            // *binPtr = curList;
            bins[sz] = curList;

            ret = reinterpret_cast<void*>(p);
        }
        else{
            // allocate from the global list(bin_a).
            // TODO : FromGlobalList
            curList = FromGlobalList(size);
            if(curList != nullptr){
                ret = curList;
            }
            else{
                // allocate from the heap.
                // XXX : 指针的类型
                ret = FromHeap(size);
                return ret;
            }
        }
    }

    return reinterpret_cast<void*>(ret + sizeof(block_info));
}





/*
 * maps new memory address using mmap system call for size
 * request > 512 bytes.
 * Requests kernel to map new memory at some place decided by kernel.
 * params: requested size in bytes.
 * returns: pointer to block allocated., NULL on failure.
 */
void * mmap_new_memory(size_t size)
{
    int num_pages =
        ((size + sizeof(block_info) - 1)/sysconf(_SC_PAGESIZE)) + 1;
    int required_page_size = sysconf(_SC_PAGESIZE) * num_pages;

    void *ret = mmap(NULL, // let kernel decide.
                     required_page_size,
                     PROT_READ | PROT_WRITE,
                     MAP_ANONYMOUS| MAP_PRIVATE,
                     -1, //no file descriptor
                     0); //offset.

    block_info b;
    b.size = (required_page_size - sizeof(block_info));
    b.next = NULL;

    ret = memcpy(ret, &b, sizeof(block_info));
    ret = ((char*)ret + sizeof(block_info));

    return ret;
}

void* allocLargeByMmap(size_t size)
{
    return mmap_new_memory(size);
}

/*
 * Performs allocation for request > 512 bytes.
 * params : requested memory size.
 * returns: pointer to allocated memory. NULL on failure.
 */
void* allocLarge(size_t size)
{
    void* res = nullptr;
    if(nullptr != bin_large){
        // 直接从threadCache中申请 不需要锁
        //pthread_mutex_lock(&global_heap_mutex);
        res = findBestFitFromLarge(size);
        //pthread_mutex_unlock(&global_heap_mutex);
    }
    if(nullptr == res){
        //TODO : by mmap
        res = allocLargeByMmap(size);
    }
    return res;
}



void* Mmalloc(size_t size)
{
    void* res = nullptr;
    // pthread_create_key 
    threadCache::InitTSD();
    threadCache* heap = threadCache::getCache();
    if(heap == nullptr){
        heap = threadCache::createCache();
    }
    // XXX : threadCache::allocate(size_t);
    if(size > 512){
        // res = alloc_large(size);
        res = allocLarge(size);
    }
    else{
        res = heap -> allocate(size);
    }
    return res;
}

/*
 * Free up the memory allocated at pointer p. It appends the block into free
 * list.
 * params: address to pointer to be freed.
 * returns: NONE.
 */
void Mfree(void *p)
{
    assert(threadCache::hasInit());
    threadCache* heap = threadCache::getCache();
    assert(heap != nullptr);
    
    if(nullptr != p)
    {
        block_info *block  = (block_info *)(p - sizeof(block_info));
        memset(p, '\0', block->size);

        block_info** BlockPtr = nullptr;
        block_info* _list = nullptr;
        // get from bin_large
        if((block -> size) > 512){
            _list = bin_large;
            BlockPtr = &bin_large;
        }
        // get from small bins
        else{
            printf("%d", block -> size);
            int index = heap -> getTheListIndex(block -> size);
            _list = heap -> freeList[index];
            BlockPtr = &_list;
        }
        
        block_info *check_bin = _list;
        // freed?
        while(check_bin != nullptr)
        {
            // yes
            if(check_bin == block)
            {
                return;
            }
            // ..
            check_bin = check_bin->next;
        }

        // attach as head to free list of corresponding bin.
        block->next = _list;
        *BlockPtr = block;
    }
    if(((char*)sbrk(0) - (char*)heap_used_memory_end) > (128 << 10))
    {
        size_t s = (128 << 10);
        if(sbrk(-s) == (void*)-1){
            perror("\n sbrk failed to freed heap.");
            return;
        }
    }
} // Mfree

} // namespace tcmalloc

} // namespace global