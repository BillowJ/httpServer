#ifndef HTTPRESPONSE_H_
#define HTTPRESPONSE_H_H

#include "Buffer.h"
#include <unistd.h>
#include <fcntl.h> // open
#include <sys/stat.h>
#include <sys/mman.h>
#include <unordered_map>

using std::string;

namespace httpServer
{
    class HttpResponse{
     public:
        HttpResponse();
        ~HttpResponse();

        void InitResponse(const string&, string&, bool, int);
        void MakeResponse(Buffer&);
     private:
        void UnmapFile();

     private:
        
        int code_;
        std::string Path_;              // 文件相对路径
        std::string SrcDir_;            // 文件绝对路径
        bool isKeepAlive_;

        char* mmFile_; 
        struct stat mmFileStat_;

        static const std::unordered_map<std::string, std::string> MimeTypes;
        static const std::unordered_map<int, std::string> CODE_STATUS;
        static const std::unordered_map<int, std::string> CODE_PATH;
    }; 
} // namespace httpServer

#endif