#ifndef EPOLLER_H_
#define EPOLLER_H_

#include <vector>
#include <sys/epoll.h>
#include <stdio.h>
#include <cassert>
#include <unistd.h>

namespace httpServer{

const int MaxEvent = 1024;

class Epoller{
public:
    explicit Epoller(int maxEvent = MaxEvent);
    ~Epoller();

    void Epoll_Init(int maxEvent, int listenNum);
    size_t Epoll_Wait(int TimeOut);
    
    int GetFd(size_t);
    uint32_t GetEvent(size_t);
    
    bool RemFd(int Fd);
    bool AddFd(int Fd, uint32_t Event);
    bool ModFd(int Fd, uint32_t Event);

private:
    int EpollFd_;
    std::vector<epoll_event> Events_;
};

} // namepsace httpServer


#endif