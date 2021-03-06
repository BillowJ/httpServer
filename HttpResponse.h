#ifndef HTTPRESPONSE_H_
#define HTTPRESPONSE_H_

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

        void UnmapFile();
        void InitResponse(const string&, string&, bool, int);
        void MakeResponse(Buffer&);
        char* File() { return mmFile_; }
        size_t FileLen() const { return mmFileStat_.st_size; }

     private:
        
        void AddStatusLine(Buffer&);
        void AddHeader(Buffer&);
        void AddContent(Buffer&);
        void ErrorHtml();
        void ErrorContent(Buffer& buff, string);
        string GetFileType();

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