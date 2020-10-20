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

    void HttpConn::Init(int fd, const sockaddr_in& addr,
    std::function<std::string(void*)> postHandler,
    const std::unordered_map<string, std::function<void(void*)>>& callBacks){
        assert(fd > 0);
        UserCount++;
        fd_ = fd;
        addr_ = addr;
        wBuffer_.RetrieveAll();
        rBuffer_.RetrieveAll();
        isClose = false;
        PostHandler_ = postHandler;
        CallBacks_ = callBacks;
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
        if(rBuffer_.ReadableBytes() == 0 ){ return false; }
        req_.Init();
        if(req_.Parse(rBuffer_)){
            // 是否调用对应的回调函数 从下层request对象获取Key进行匹配
            if(req_.Method() == "GET"){
                for(auto& item : req_.GetKeys())
                {
                    if(CallBacks_.count(item.first) == 1){
                        auto CallFunc = CallBacks_[item.first];
                        CallFunc((void*)&item.second);
                    }
                }
            }
            else if(req_.Method() == "POST"){
                if(PostHandler_){
                    // 将body传入到业务层提供的解析函数中
                    // 疑惑: 解析完应该返回什么数据？
                    std::string ans = PostHandler_((void*)&req_.GetPostBody());
                }
            }
            //TODO : 判断是否业务层已经对其进行过处理，若有则更改response的生成策略

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
