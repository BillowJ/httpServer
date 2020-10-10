#ifndef SERVER_H_
#define SERVER_H_
#include <memory>
#include <vector>

#include "Utils.h"
#include "ThreadPool.h"
#include "Epoller.h"
#include "HttpConn.h"

namespace httpServer
{
    class Server{
     public:
        Server(int Port, int TrigerMode);
        ~Server();
        void Start();

     private:
        bool InitSocket();
        void InitEvenMode(int);
        void AddClient(int Fd, struct sockaddr_in&);
        void HandleListen();
        void HandleRead(HttpConn*);
        void HandleWrite(HttpConn*);
        void CloseConn(HttpConn*);

     
     private:
        char* SrcDir_;
        int ListenPort_;
        int ListenFd_;
        bool isStart_;

        uint32_t listenEvent_;
        uint32_t connEvent_;

        std::unique_ptr<Epoller> Epoller_;
        std::unique_ptr<ThreadPool> ThreadPool_;
        std::unordered_map<int , HttpConn> Users_;
    };

} // namespace httpServer


#endif