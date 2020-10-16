#include "HttpConn.h"


namespace httpServer
{
    const char* HttpConn::SrcDir_;
    std::atomic<int> HttpConn::UserCount;
    std::string FilePath;

    HttpConn::HttpConn() : fd_(-1), isClose(true) {
        addr_ = { 0 };
    }
    HttpConn::HttpConn(int fd, const struct sockaddr_in& addr) : fd_(fd), addr_(addr){
        assert(fd > 0);
        UserCount++;
        wBuffer_.RetrieveAll();
        rBuffer_.RetrieveAll();
        isClose = false;
    }
    HttpConn::~HttpConn() {
        this -> Close();
    }

    void HttpConn::Init(int fd, const sockaddr_in& addr){
        assert(fd > 0);
        UserCount++;
        fd_ = fd;
        addr_ = addr;
        wBuffer_.RetrieveAll();
        rBuffer_.RetrieveAll();
        isClose = false;
    }

    void HttpConn::Close(){
        // TODO: response 解除请求文件映射
        rsp_.UnmapFile();
        if(isClose == false){
            isClose = true;
            UserCount--;
            close(fd_);
        }
    }
    
    int HttpConn::GetFd() const {
    return fd_;
    }

    struct sockaddr_in HttpConn::GetAddr() const {
        return addr_;
    }

    const char* HttpConn::GetIp() const {
        return inet_ntoa(addr_.sin_addr);
    }

    int HttpConn::GetPort() const {
        return addr_.sin_port;
    }


    // 从应用层缓存区写到内核缓存区
    ssize_t HttpConn::write(int* Errno){
        ssize_t len = -1;
        do
        {
            len = writev(fd_, Iov_, IovCnt_);
            if(len <= 0){
                *Errno = errno;
                break;
            }
            if(Iov_[0].iov_len + Iov_[1].iov_len == 0) break;
            else if(static_cast<size_t>(len) > Iov_[0].iov_len){
                Iov_[1].iov_base = (uint8_t*)Iov_[1].iov_base + (len - Iov_[0].iov_len);
                Iov_[1].iov_len -= (len - Iov_[0].iov_len);
                if(Iov_[0].iov_len){
                    wBuffer_.RetrieveAll();
                    Iov_[0].iov_len = 0;
                }
            }
            else{
                Iov_[0].iov_len -= len;
                Iov_[0].iov_base = (uint8_t*)Iov_[0].iov_base + len;
                wBuffer_.Retrieve(len);
            }
            
        } while (true);
        return len;
    }

    // 读内核缓存区到应用层缓存区
    ssize_t HttpConn::read(int* Errno){
        ssize_t len = -1;
        ssize_t allSize = 0;
        while (true)
        {
            len = rBuffer_.ReadFd(fd_, Errno);
            allSize += len;
            if(len <= 0){ 
                // *Errno = errno;
                break;
            }
        }
        return len;
    }

    bool HttpConn::Process(){
        // TODO: 初始化request 读取rBuffer解析request 
        // TODO: 构造response到wBuffer
        if(rBuffer_.ReadableBytes() == 0 ){ return false; }
        req_.Init();
        if(req_.Parse(rBuffer_)){
            rsp_.InitResponse(SrcDir_, req_.Path(), req_.IsKeepAlive(), 200);
        }
        else{
            rsp_.InitResponse(SrcDir_, req_.Path(), false, 400);
        }
        rsp_.MakeResponse(wBuffer_);
        /* 响应头 */
        Iov_[0].iov_base = const_cast<char*>(wBuffer_.Peek());
        Iov_[0].iov_len = wBuffer_.ReadableBytes();
        IovCnt_ = 1;

        /* 文件 */
        if(rsp_.FileLen() > 0  && rsp_.File()) {
            Iov_[1].iov_base = rsp_.File();
            Iov_[1].iov_len = rsp_.FileLen();
            IovCnt_ = 2;
        }
        return true;
    }


} // namespace httpServer
