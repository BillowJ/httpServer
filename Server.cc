#include "Server.h"


namespace httpServer
{
    Server::Server(int Port, int TrigeMode) : ListenPort_(Port), isStart_(true), 
    Epoller_(new Epoller), ThreadPool_(new ThreadPool){
            SrcDir_ = getcwd(nullptr, 256);
            assert(SrcDir_);
            strncat(SrcDir_, "/Base/", 26);
            HttpConn::UserCount = 0;
            HttpConn::SrcDir_ = SrcDir_;

            InitEvenMode(TrigeMode);
            if(!InitSocket()) { isStart_ = false;}
        }
} // namespace httpServer
