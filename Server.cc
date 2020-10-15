#include "Server.h"


namespace httpServer
{
    Server::Server(int Port, int ThreadNum) : ListenPort_(Port), isStart_(true), 
    Epoller_(new Epoller){
            SrcDir_ = getcwd(nullptr, 256);
            assert(SrcDir_);
            strncat(SrcDir_, "/Base/", 26);
            HttpConn::UserCount = 0;
            HttpConn::SrcDir_ = SrcDir_;
            ThreadPool::ThreadPool_Create(ThreadNum);
            InitEvenMode();
            if(!InitSocket()) { isStart_ = false;}
        }

    Server::~Server(){
        close(ListenFd_);
        isStart_ = false;
        ThreadPool::ThreadPool_Destroy();
        // free(SrcDir_);
    }

    void Server::Start(){
        int timeOut = -1;
        if(!isStart_) {
            perror("Server Start Failed\n");
            return;
        }
        while(isStart_){
            int eventCount = Epoller_ -> Epoll_Wait(timeOut);
            for(int idx = 0; idx < eventCount; idx++){
                int fd = Epoller_ -> GetFd(idx);
                uint32_t event = Epoller_ -> GetEvent(idx);
                if(fd == ListenFd_){
                    HandleListen();
                }
                else if(event & (EPOLLHUP | EPOLLERR | EPOLLRDHUP)){
                    // TODO : close the conn
                }
                else if(event & EPOLLIN){
                    HandleRead(&Users_[fd]);
                }
                else if(event & EPOLLOUT){
                    HandleWrite(&Users_[fd]);
                }
                else{
                    perror("不知道什么事件\n");
                    return;
                }
            }
        }
    }

    bool Server::Close(){
        perror("some error about Init Socket");
        close(ListenFd_);
        return false;
    }

    void Server::InitEvenMode(){
        connEvent_ = EPOLLONESHOT | EPOLLRDHUP | EPOLLET;
        listenEvent_ = EPOLLET | EPOLLRDHUP;
    }


    bool Server::InitSocket(){
        
        if(ListenPort_ > 65535 || ListenPort_ < 1024){
            perror("PORT INVAILD\n");
            return false;
        }
        
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(ListenPort_);

        struct linger optLinger = {0};
        if(OpenLinger_){
            optLinger.l_linger = 1;
            optLinger.l_onoff = 1;
        }

        ListenFd_ = socket(AF_INET, SOCK_STREAM, 0);
        if(ListenFd_ < 0) {
            return Close();
        }

        int ret = setsockopt(ListenFd_, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
        if(ret < 0) {
            return Close();
        }
        
        int optval = 1;
        ret = setsockopt(ListenFd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
        if(ret < 0) { return Close(); }
        
        ret = bind(ListenFd_, (struct sockaddr*)&addr, sizeof(addr));
        if(ret< 0) { return Close(); }
        
        if((listen(ListenFd_, 6)) < 0){
            return Close();
        }

        ret = Epoller_ -> AddFd(ListenFd_, listenEvent_ | EPOLLIN);
        if(ret < 0) { return Close(); }

        ::SetNonBlocking(ListenFd_);
        return true;
    }

    void Server::SendError(int fd, const char*info) {
        assert(fd > 0);
        int ret = send(fd, info, strlen(info), 0);
        if(ret < 0) {
            perror("send error to client[%d] error!", fd);
        }
        close(fd);
    }

    void Server::AddClient(int fd, struct sockaddr_in& addr){
        assert(fd > 0);
        Users_[fd].Init(fd, addr);
        // TODO : add timer
        Epoller_ -> AddFd(fd, EPOLLIN | connEvent_);
        ::SetNonBlocking(fd);
    }

    void Server::HandleListen(){

        struct sockaddr_in addr;
        socklen_t addrLen = sizeof(addr);
        int clientFd;
        while(clientFd = accept(ListenFd_, (sockaddr*)&addr, &addrLen) > 0){
            if(clientFd <= 0) { return; }
            else if(HttpConn::UserCount >= MaxFd_){
                SendError(clientFd, "Server busy!");
                return;
            }
            AddClient(clientFd, addr);
        }
    }

    void Server::HandleRead(HttpConn* client){
        assert(client);
        ThreadPool::ThreadPool_AddTask(std::bind(&Server::OnRead, this, client));
    }

    void Server::HandleWrite(HttpConn* client){
        assert(client);
        ThreadPool::ThreadPool_AddTask(std::bind(&Server::OnWrite, this, client));   
    }

    void Server::CloseConn(HttpConn* client){
        assert(client);
        printf("Client[%d] quit!", client -> GetFd());
        Epoller_ ->RemFd(client -> GetFd());
        client -> Close();
    }

    void Server::OnRead(HttpConn* client){
        int errno;
        int ret;
        ret = client -> read(&errno);
        if(ret <= 0 && errno != EAGAIN){
            CloseConn(client);
            return;
        }
        OnProcess(client);
    }
    
    void Server::OnWrite(HttpConn* client){

    }

    void Server::OnProcess(HttpConn* clien){

    }



} // namespace httpServer
