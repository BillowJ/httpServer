#ifndef REQUEST_H_
#define REQUEST_H_
#include <string>
#include <unistd.h> //close
#include <arpa/inet.h>
#include "Buffer.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

namespace httpServer
{

    class HttpConn{
    public:
        HttpConn();
        HttpConn(int fd, const struct sockaddr_in& addr);
        ~HttpConn() = default;

        void Init(int fd, const sockaddr_in& addr);
        void Close();
        bool Process();

        ssize_t read(int* Errno);
        ssize_t write(int* Errno);
        ssize_t WaitToWrite() const{ return Iov_[0].iov_len + Iov_[1].iov_len; }
        bool isKeepAlive() const { return req_.IsKeepAlive(); }

        int GetFd() const;
        int GetPort() const;
        const char* GetIp() const;
        struct sockaddr_in GetAddr() const;

    public:
        static const char* SrcDir_;
        static std::atomic<int> UserCount;
        static std::string FilePath;
    
    private:
        int fd_;
        int IovCnt_;
        bool isClose;

        struct sockaddr_in addr_;
        struct iovec Iov_[2];

        HttpRequest req_;
        HttpResponse rsp_;

        Buffer rBuffer_;
        Buffer wBuffer_;
        
    };
} // namespace httpServer


#endif