#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include <pthread.h>
#include <vector>
#include <queue>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <cassert>

using std::function;

namespace httpServer
{
    /*
    struct ThreadTask
    {
        function<void(void*)> func;
        void* arg;
    };
    */
    class HttpConn;
    class ThreadPool{
    private:

        /* POSIX Thread Library */
        static pthread_mutex_t Mutex_;
        static pthread_cond_t Cv_;
        
        /* C++11 Standard */
        // static std::mutex Mutex_;
        // static std::condition_variable Cv_;
        
        static bool ShutDownButton;
        static std::vector<pthread_t> threads_;
        static std::queue<function<void()>> tasks_;
        static size_t ThreadNum_;
        
    private:

        static void* ThreadPool_WorkerFunc(void*);
    
    public:
         
        // template<class T>
        // static void ThreadPool_AddTask(T&& Func);

        static void ThreadPool_AddTask(std::function<void()>);

        static int  ThreadPool_Destroy();
        static bool ThreadPool_Create(size_t threadNum = 3);
    };

} // namespace httpServer

#endif