#ifndef HTTPCONN_H_
#define HTTPCONN_H_
#include <string>
#include <unistd.h> //close
#include <queue>
#include <arpa/inet.h>
#include "Buffer.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

namespace httpServer
{
    /* 负责生成Response和接受Request及其客户端的IO处理逻辑*/
    class HttpConn{
    public:
        HttpConn();
        HttpConn(int fd, const struct sockaddr_in& addr);
        ~HttpConn();

        void Init(int fd, const sockaddr_in& addr, std::function<std::string(void*)>,
                  const std::unordered_map<string, std::function<void(void*)>>&, int);
        void Close();
        bool Process();

        ssize_t read(int* Errno);
        ssize_t write(int* Errno);
        ssize_t WaitToWrite() const{ return Iov_[0].iov_len + Iov_[1].iov_len; }
        bool isKeepAlive() const { return req_.IsKeepAlive(); }

        int GetFd() const;
        int GetPort() const;
        const char* GetIp() const;
        size_t GetThreadIdx() const { return BindThreadIdx_; }
        struct sockaddr_in GetAddr() const;

        void SetCallBack(const std::string& key, std::function<void(void*)> callback)
        {
            CallBacks_[key] = callback;
        }

        void SetPostHandler(const std::function<std::string(void*)>& postHandler){
            this -> PostHandler_ = postHandler;
        }

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
        
        //GET的Key和回调的映射
        std::unordered_map<string, std::function<void(void*)>> CallBacks_;

        // POST的响应处理函数
        std::function<std::string(void*)> PostHandler_;

        // 每个客户端的所有请求都由一个线程完成，避免线程之间的竞争。
        size_t BindThreadIdx_;

        HttpRequest req_;
        HttpResponse rsp_;

        Buffer rBuffer_;
        Buffer wBuffer_;
        
    };
} // namespace httpServer


#endif