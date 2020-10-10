#include "HttpResponse.h"

using namespace httpServer;


const std::unordered_map<string, string>  httpServer::HttpResponse::MimeTypes = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};

const std::unordered_map<int, string> httpServer::HttpResponse::CODE_STATUS = {
    {200, "OK"},
    {400, "Bad Request"},
    {404, "Not Found"},
    {403, "Forbidden"}
};

const std::unordered_map<int, string> httpServer::HttpResponse::CODE_PATH = {
    {200, "index.html"},
    {400, ""},
    {403, ""},
    {404, ""}
};

HttpResponse::HttpResponse(){
    code_ = -1;
    Path_ = SrcDir_ = "";
    isKeepAlive_ = false;
    mmFile_ = nullptr; 
    mmFileStat_ = { 0 };
    // ...
}

HttpResponse::~HttpResponse(){
    UnmapFile();
}

void HttpResponse::InitResponse(const string& SrcDir, string& Path, bool isKeepAlive, int code){
    assert(SrcDir != "");
    SrcDir_ = SrcDir;
    code_ = code;
    isKeepAlive_ = isKeepAlive;
    Path_ = Path;
    mmFile_ = nullptr;
    mmFileStat_ = {0};
}

