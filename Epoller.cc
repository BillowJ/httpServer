#include "Epoller.h"


namespace httpServer
{
    
    Epoller::Epoller(int maxEvent) : EpollFd_(epoll_create(1024)), Events_(maxEvent){
        assert(EpollFd_ >= 0 && Events_.size() > 0);
    }

    Epoller::~Epoller(){
        close(EpollFd_);
    }

    void Epoller::Epoll_Init(int maxEvent, int listenNum){
        assert(EpollFd_ == 0);
        EpollFd_ = epoll_create(listenNum);
        if (EpollFd_ == -1)
        {
            perror("Epoll Create Faild \n");
            return;
        }
        Events_.resize(maxEvent);
    }

    size_t Epoller::Epoll_Wait(int TimeOut){
        int count_ = epoll_wait(EpollFd_, &Events_[0], static_cast<int>(Events_.size()), TimeOut);
        if(count_ < 0) { perror("Epoll_Wait Faild\n"); }
        return count_;
    }

    // 获取对应下标的Fd
    int Epoller::GetFd(size_t idx){
        assert(idx >= 0 && idx < Events_.size());
        int resFd = Events_[idx].data.fd;
        return resFd; 
    }

    // 获取对应下标的事件
    uint32_t Epoller::GetEvent(size_t idx){
        assert(idx >= 0 && idx < Events_.size());
        struct epoll_event ev;
        ev = Events_[idx];
        return ev.events;
    }

    bool Epoller::RemFd(int Fd){
        if(Fd < 0) return false;
        epoll_event ev = {0};
        return 0 == epoll_ctl(EpollFd_, EPOLL_CTL_DEL, Fd, &ev);
        return true;
    }

    bool Epoller::AddFd(int Fd, uint32_t Event){
        if(Fd < 0) return false;
        epoll_event ev;
        ev.data.fd = Fd;
        ev.events = Event;
        return 0 == epoll_ctl(EpollFd_, EPOLL_CTL_ADD, Fd, &ev);
    }
    
    bool Epoller::ModFd(int Fd, uint32_t Event){
        if(Fd < 0) return false;
        struct epoll_event ev;
        ev.data.fd = Fd;
        ev.events = Event;
        return 0 == epoll_ctl(EpollFd_, EPOLL_CTL_MOD, Fd, &ev);
    }
} // namespace httpServer
