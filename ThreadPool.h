#include <pthread.h>
#include <vector>
#include <queue>
#include <condition_variable>
#include <functional>
#include <mutex>

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

    class ThreadPool{
    private:
        typedef std::function<void()> ThreadTask;

        // struct ThreadTask
        // {
        //     function<void(void*)> CallBackFunc;
        //     void* Args;
        // };
        
        /* POSIX Thread Library */
        static pthread_mutex_t Mutex_;
        static pthread_cond_t Cv_;
        
        /* C++11 Standard */
        // static std::mutex Mutex_;
        // static std::condition_variable Cv_;
        

        static std::vector<pthread_t> threads_;
        static std::queue<ThreadTask> tasks_;
        static size_t threadNum;
        
    public:
        static int ThreadPool_Create(size_t threadNum = 3, size_t taskNum = 5);
        static int ThreadPool_AddTask(std::function<void(void*)> Func, void* Arg);
        static int ThreadPool_Destroy();

        static void* ThreadPool_WorkerFunc(void*);
    };

} // namespace httpServer
