#include "Epoller.h"
#include <stdio.h>
namespace httpServer
{
    int Epoller::EpollFd_ = 0;
    const size_t Epoller::MaxEvent_ = 1024;
    std::vector<epoll_event> Epoller::events_;
    
    void Epoller::Epoll_Init(int maxEvent, int listenNum){
        EpollFd_ = epoll_create(listenNum);
        if (EpollFd_ == -1)
        {
            perror("Epoll Create Faild \n");
            return;
        }
        events_.resize(maxEvent);
    }

    void Epoller::Epoll_Acception(int listenFd, int epollFd){
        
    }



    size_t Epoller::Epoll_Wait(int ListenFd, int MaxEvent, int TimeOut){
        int count_ = epoll_wait(EpollFd_, &events_[0], static_cast<int>(events_.size()), TimeOut);
        if(count_ < 0) { perror("Epoll_Wait Faild\n"); }
        std::vector<epoll_event*> events = Epoller::Epoll_EventsHandler(ListenFd, count_, events_);
        for(auto& singleEvent : events){
            if(singleEvent->data.fd == ListenFd){
                Epoll_Acception(ListenFd, EpollFd_);
            }

            
        }
    }
} // namespace httpServer
