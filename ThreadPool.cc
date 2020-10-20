#include "ThreadPool.h"


namespace httpServer{

    std::vector<pthread_t> ThreadPool::threads_;
    std::queue<function<void()>> ThreadPool::tasks_;
    bool ThreadPool::ShutDownButton = false;

    pthread_mutex_t ThreadPool::Mutex_ = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t ThreadPool::Cv_ = PTHREAD_COND_INITIALIZER;
    size_t ThreadPool::ThreadNum_ = 0;

    bool ThreadPool::ThreadPool_Create(size_t threadNum){
        do{
            if(threadNum > 12 || threadNum < 0){
                threadNum = 6;
            }
            threads_.resize(threadNum);

            for(size_t idx = 0;idx < threadNum; idx++)
            {
                
                if(pthread_create(&threads_[idx], NULL, ThreadPool_WorkerFunc, (void*)(0)) != 0)
                {
                    continue;
                    // return false;
                }

                ThreadNum_++;
            }
        } while(0);

        return true;
    }

    void* ThreadPool::ThreadPool_WorkerFunc(void* args){

        while(true){
            pthread_mutex_lock(&Mutex_);
            while(tasks_.empty() || ShutDownButton){
                pthread_cond_wait(&Cv_, &Mutex_);
            }

            if(ShutDownButton){
                break;
            }
            auto task = tasks_.front();
            tasks_.pop();
            pthread_mutex_unlock(&Mutex_);
            task();
        }

        pthread_exit(NULL);
        printf("Worker Threads Finish! \n");
        return NULL;
    }
    
    /*
    template<class T>
    void ThreadPool::ThreadPool_AddTask(T&& task){
        pthread_mutex_lock(&Mutex_);
        {
            tasks_.emplace(std::forward<T>(task));
        }
        pthread_mutex_unlock(&Mutex_);
        pthread_cond_signal(&Cv_);
    }
    */
    void ThreadPool::ThreadPool_AddTask(std::function<void()> task){
        pthread_mutex_lock(&Mutex_);
        {
            // 减少copy
            tasks_.push(std::move(task));
        }
        
        pthread_cond_signal(&Cv_);
        pthread_mutex_unlock(&Mutex_);
    }

    int ThreadPool::ThreadPool_Destroy(){
        assert(!ShutDownButton);
        if(pthread_mutex_lock(&Mutex_) != 0){
            perror("ThreadPool_line71: lock Error \n");
            return -1;
        }

        ShutDownButton = true;
        
        if((pthread_cond_broadcast(&Cv_) != 0) ||
            (pthread_mutex_unlock(&Mutex_) != 0)){
                perror("Error!\n");
                return -1;
        }
        
        //Free Threads
        for(size_t i = 0; i < ThreadNum_; i++){
            if(pthread_join(threads_[i], NULL) != 0){
                perror("Thread Join Error");
                return -1;
            }
        }

        // Free Memory
        do
        {
            pthread_mutex_lock(&Mutex_);
            pthread_mutex_destroy(&Mutex_);
            pthread_cond_destroy(&Cv_);

        } while (false);
        
        return 0;
    }
}