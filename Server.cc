#include "Server.h"


namespace httpServer
{
    Server::Server(int Port, int ThreadNum) : ListenPort_(Port), isStart_(true), 
    Epoller_(new Epoller()){
            SrcDir_ = getcwd(nullptr, 256);
            assert(SrcDir_);
            strncat(SrcDir_, "/Base/", 26);
            HttpConn::UserCount = 0;
            HttpConn::SrcDir_ = SrcDir_;
            ThreadPool::ThreadPool_Create(ThreadNum);
            ThreadNum_ = ThreadNum;
            InitEvenMode();
            if(!InitSocket()) { printf("error!\n"); isStart_ = false;}
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
                    printf("HandleListen");
                    HandleListen();
                }
                else if(event & (EPOLLHUP | EPOLLERR | EPOLLRDHUP)){
                    // TODO : close the conn
                    CloseConn(&Users_[fd]);
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
        if(ListenFd_ < 0) 
        {
            return false;
        }

        int ret = setsockopt(ListenFd_, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
        if(ret < 0) 
        {
            close(ListenFd_);
            return false;
        }
        
        int optval = 1;
        ret = setsockopt(ListenFd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
        if(ret < 0) 
        {
            close(ListenFd_);
            return false;
        }
        
        ret = bind(ListenFd_, (struct sockaddr*)&addr, sizeof(addr));
        if(ret< 0) 
        {
            close(ListenFd_);
            return false;    
        }
        
        if((listen(ListenFd_, 6)) < 0)
        {
            close(ListenFd_);
            return false;
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
            printf("send error to client[%d] error!", fd);
        }
        close(fd);
    }

    void Server::AddClient(int fd, struct sockaddr_in& addr){
        assert(fd > 0);
        // 若有业务层提供的相应函数 直接传入进行初始化
        Users_[fd].Init(fd, addr, PostParserHandler_, CallBacks_, ThreadNum_);
        // TODO : add timer
        ::SetNonBlocking(fd);
        Epoller_ -> AddFd(fd, (EPOLLIN | connEvent_));
    }

    void Server::HandleListen(){

        struct sockaddr_in addr;
        socklen_t addrLen = sizeof(addr);
        // int clientFd;
        do{
            int clientFd = accept(ListenFd_, (sockaddr*)&addr, &addrLen);
            if(clientFd <= 0) return;
            else if(HttpConn::UserCount >= MaxFd_){
                SendError(clientFd, "Server busy!");
                return;
            }

            AddClient(clientFd, addr);
        } while(listenEvent_ & EPOLLET);
    }

    void Server::HandleRead(HttpConn* client){
        assert(client);
        ThreadPool::ThreadPool_AddTask(std::bind(&Server::OnRead, this, client), client);
    }

    void Server::HandleWrite(HttpConn* client){
        assert(client);
        ThreadPool::ThreadPool_AddTask(std::bind(&Server::OnWrite, this, client), client);   
    }

    void Server::CloseConn(HttpConn* client){
        assert(client);
        printf("Client[%d] quit!", client -> GetFd());
        Epoller_ ->RemFd(client -> GetFd());
        client -> Close();
    }

    void Server::OnRead(HttpConn* client){
        int Errno;
        int ret;
        ret = client -> read(&Errno);
        if(ret <= 0 && Errno != EAGAIN){
            CloseConn(client);
            return;
        }
        OnProcess(client);
    }
    
    void Server::OnWrite(HttpConn* client){
        int Errno;
        int ret = client ->write(&Errno);
        if(client ->WaitToWrite() == 0){
            if(client ->isKeepAlive()){
                OnProcess(client);
            }
        }
        else if(ret < 0){
            if(Errno == EAGAIN){
                Epoller_ ->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
                return;
            }
        }
        CloseConn(client);
    }

    void Server::OnProcess(HttpConn* client){
        if(client -> Process()){
            //write
            Epoller_ ->ModFd(client -> GetFd(), connEvent_ | EPOLLOUT);
        }
        // keep-alive
        else{
            Epoller_ ->ModFd(client -> GetFd(), connEvent_ | EPOLLIN);
        }
    }

    bool Server::ResigterHandler(const string& key, std::function<void(void*)> callback ){
        if(key.empty()) return false;
        
        // Key的格式化
        string ans;
        for(size_t i = 0; i < key.size(); i++){
            char ch = key[i];
            int ascii = (int)(ch);
            ans = ans + "%" + std::to_string(ascii);
        }

        CallBacks_[ans] = callback;
        
        // 分发至现有的连接
        for(auto& item : Users_){
            auto singleConn = item.second;
            singleConn.SetCallBack(ans, callback);
        }

        return true;
    }

    bool Server::SetPostParseHandler(const std::function<string(void*)>& callback){
        PostParserHandler_ = callback;
        
        // 更新现有的连接
        for(auto& item : Users_){
            auto singleConn = item.second;
            singleConn.SetPostHandler(PostParserHandler_);
        }
    }
} // namespace httpServer
