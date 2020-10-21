#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include <pthread.h>
#include <vector>
#include <queue>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <cassert>
#include <unordered_map>
#include <memory>
#include "HttpConn.h"

using std::function;

namespace httpServer
{
    
    // class HttpConn;
    class ThreadPool;

    class Thread{
    public:
        Thread() { cv_ = PTHREAD_COND_INITIALIZER; }
        pthread_t pid_;
        pthread_cond_t cv_;
        std::queue<std::function<void()>> Tasks_;
        bool start(void* arg);
    };
    
    class ThreadPool{
    private:

        /* POSIX Thread Library */
        static pthread_mutex_t Mutex_;
        // static pthread_cond_t Cv_;
        
        /* C++11 Standard */
        // static std::mutex Mutex_;
        // static std::condition_variable Cv_;
        
        static bool ShutDownButton;
        static std::vector<pthread_t> threads_;
        static std::queue<function<void()>> tasks_;
        static size_t ThreadNum_;
        // static std::unordered_map<size_t, std::queue<std::function<void()> > Tasks_;
        static std::vector<std::shared_ptr<Thread>> Threads_;

    private:

        
    
    public:
        static void* ThreadPool_WorkerFunc(void*);
        static void ThreadPool_AddTask(std::function<void()>, HttpConn*);
        static int  ThreadPool_Destroy();
        static bool ThreadPool_Create(size_t threadNum = 5);
    };

} // namespace httpServer





#endif