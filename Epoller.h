#ifndef EPOLLER_H_
#define EPOLLER_H_

#include <vector>
#include <sys/epoll.h>

namespace httpServer{

const int MaxEvent = 1024;

class Epoller{
public:
    static void Epoll_Init(int, int);
    static size_t Epoll_Wait(int ListenFd, int MaxEvents, int TimeOut);
    static void Epoll_Acception(int ListenFd, int EpollFd);
    
    static std::vector<epoll_event*> Epoll_EventsHandler(int, size_t, std::vector<epoll_event>&);
    
    static bool RemFd();
    static bool AddFd();
    static bool ModFd();

private:
    static int EpollFd_;
    static const size_t MaxEvent_;
    static std::vector<epoll_event> events_;
};

} // namepsace httpServer


#endif