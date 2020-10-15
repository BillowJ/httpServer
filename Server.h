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
      //   Server(int Port, int TrigerMode);
        Server(int Port, int Threads);
        ~Server();
        void Start();

     private:
        bool InitSocket();
        void InitEvenMode();
        void AddClient(int Fd, struct sockaddr_in&);
        void HandleListen();
        void HandleRead(HttpConn*);
        void HandleWrite(HttpConn*);
        void OnRead(HttpConn*);
        void OnWrite(HttpConn*);
        void OnProcess(HttpConn*);
        void CloseConn(HttpConn*);
        bool Close();
        void SendError(int, const char*);
     
     private:
        char* SrcDir_;
        int ListenPort_;
        int ListenFd_;
        bool isStart_;
        bool OpenLinger_;

        uint32_t listenEvent_;
        uint32_t connEvent_;
        static const int MaxFd_ = 65535;
        std::unique_ptr<Epoller> Epoller_;
        std::unordered_map<int , HttpConn> Users_;
     // std::unique_ptr<ThreadPool> ThreadPool_;
    };

} // namespace httpServer


#endif