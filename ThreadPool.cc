#include "ThreadPool.h"


namespace httpServer{

    std::vector<pthread_t> ThreadPool::threads_;
    std::queue<function<void()>> ThreadPool::tasks_;
    bool ThreadPool::ShutDownButton = false;
    std::vector<std::shared_ptr<Thread>> ThreadPool::Threads_;

    pthread_mutex_t ThreadPool::Mutex_ = PTHREAD_MUTEX_INITIALIZER;

    // pthread_cond_t ThreadPool::Cv_ = PTHREAD_COND_INITIALIZER;
    size_t ThreadPool::ThreadNum_ = 0;

    // bool ThreadPool::ThreadPool_Create(size_t threadNum){
    //     do{
    //         if(threadNum > 12 || threadNum < 0){
    //             threadNum = 6;
    //         }
    //         threads_.resize(threadNum);

    //         for(size_t idx = 0;idx < threadNum; idx++)
    //         {
                
    //             if(pthread_create(&threads_[idx], NULL, ThreadPool_WorkerFunc, (void*)(idx)) != 0)
    //             {
    //                 continue;
    //                 // return false;
    //             }

    //             ThreadNum_++;
    //         }
    //     } while(0);

    //     return true;
    // }
    
    void* ThreadPool::ThreadPool_WorkerFunc(void* arg){
        Thread* thread = (Thread*)arg;
        while (true)
        {
            pthread_mutex_lock(&Mutex_);
            while(thread->Tasks_.size() == 0 || ShutDownButton){
                pthread_cond_wait(&(thread->cv_), &Mutex_);
            }
            if(ShutDownButton){
                break;
            }
            auto task = thread -> Tasks_.front();
            thread -> Tasks_.pop();
            task();
            pthread_mutex_unlock(&Mutex_);
        }
        pthread_exit(NULL);
        return NULL;
    }

    bool ThreadPool::ThreadPool_Create(size_t ThreadNum){
        do{
            if(ThreadNum > 12 || ThreadNum < 0){
                ThreadNum = 6;
            }
            
            Threads_.resize(6);
            // Threads_.clear();
            for(size_t i = 0; i < 6; i++){
                std::shared_ptr<Thread> p(new Thread());
                Threads_[i] = p;
                // Threads_.push_back(p);
                bool ret = p -> start((void*)(p.get()));
                if(!ret) return false;
                // pthread_create(&(Threads_[i]->pid_), NULL, ThreadPool_WorkerFunc, (void*)(Threads_[i].get()));
                ThreadNum_++;
            }

        } while(false);
        return true;
    }

    

    /*
    void* ThreadPool::ThreadPool_WorkerFunc(void* arg){
        size_t IdxOfThread = *reinterpret_cast<size_t*>(arg);

        while(true){
            pthread_mutex_lock(&Mutex_);
            while(Tasks_[IdxOfThread].empty() || ShutDownButton){
                pthread_cond_wait(&Cv_, &Mutex_);
            }

            if(ShutDownButton){
                break;
            }
            // auto task = tasks_.front();
            // tasks_.pop();
            auto task = Tasks_[IdxOfThread].front();
            Tasks_[IdxOfThread].pop();

            pthread_mutex_unlock(&Mutex_);
            task();
        }

        pthread_exit(NULL);
        printf("Worker Threads Finish! \n");
        return NULL;
    }
    */
    
    // void ThreadPool::ThreadPool_AddTask(std::function<void()> task, HttpConn* client){
    //     pthread_mutex_lock(&Mutex_);
    //     {
    //         size_t idx = client -> GetThreadIdx();
    //         // 减少copy
    //         // tasks_.push(std::move(task));
    //         Tasks_[idx].push(std::move(task));
            
    //     }
        
    //     pthread_cond_broadcast(&Cv_);
    //     pthread_mutex_unlock(&Mutex_);
    // }

    void ThreadPool::ThreadPool_AddTask(std::function<void()> task, HttpConn* client){
        pthread_mutex_lock(&Mutex_);
        {
            size_t idx = client -> GetThreadIdx();
            std::shared_ptr<Thread> p = Threads_[idx];
            p ->Tasks_.push(std::move(task));
            pthread_cond_signal(&(p -> cv_));
        }
        pthread_mutex_unlock(&Mutex_);
    }

    int ThreadPool::ThreadPool_Destroy(){
        assert(!ShutDownButton);
        if(pthread_mutex_lock(&Mutex_) != 0){
            perror("ThreadPool_line71: lock Error \n");
            return -1;
        }

        ShutDownButton = true;
        
        for(auto& item : Threads_){
            pthread_cond_signal(&(item -> cv_));

        }
        
        //Free Threads
        for(size_t i = 0; i < ThreadNum_; i++){
            pthread_t t = Threads_[i] -> pid_;
            if(pthread_join(t, NULL) != 0){
                perror("Thread Join Error");
                return -1;
            }
            pthread_cond_destroy(&Threads_[i]->cv_);
            Threads_[i].reset();
        }

        // Free Memory
        do
        {
            pthread_mutex_lock(&Mutex_);
            pthread_mutex_destroy(&Mutex_);
            // pthread_cond_destroy(&Cv_);

        } while (false);
        
        return 0;
    }

    // int ThreadPool::ThreadPool_Destroy(){
    //     assert(!ShutDownButton);
    //     if(pthread_mutex_lock(&Mutex_) != 0){
    //         perror("ThreadPool_line71: lock Error \n");
    //         return -1;
    //     }

    //     ShutDownButton = true;
        
    //     if((pthread_cond_broadcast(&Cv_) != 0) ||
    //         (pthread_mutex_unlock(&Mutex_) != 0)){
    //             perror("Error!\n");
    //             return -1;
    //     }
        
    //     //Free Threads
    //     for(size_t i = 0; i < ThreadNum_; i++){
    //         if(pthread_join(threads_[i], NULL) != 0){
    //             perror("Thread Join Error");
    //             return -1;
    //         }
    //     }

    //     // Free Memory
    //     do
    //     {
    //         pthread_mutex_lock(&Mutex_);
    //         pthread_mutex_destroy(&Mutex_);
    //         pthread_cond_destroy(&Cv_);

    //     } while (false);
        
    //     return 0;
    // }

    bool Thread::start(void* arg){
           int ret = pthread_create(&pid_, NULL, ThreadPool::ThreadPool_WorkerFunc, arg);
           if(ret != 0) return false;
           return true;
    }
}