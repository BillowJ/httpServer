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

const std::unordered_map<int, string> httpServer::HttpResponse::CODE_STATUS({
    {200, "OK"},
    {400, "Bad Request"},
    {404, "Not Found"},
    {403, "Forbidden"}
});

const std::unordered_map<int, string> httpServer::HttpResponse::CODE_PATH = {
    {200, "/index.html"},
    {400, "/400.html"},
    {403, "/403.html"},
    {404, "/404.html"}
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

void HttpResponse::MakeResponse(Buffer& wbuff){
    if(stat((SrcDir_+Path_).data(), &mmFileStat_) < 0 || S_ISDIR(mmFileStat_.st_mode)){
        code_ = 404;
    }
    else if(!(mmFileStat_.st_mode & S_IROTH)){
        code_ = 403;
    }
    else if(code_ == -1){
        code_ = 200;
    }
    ErrorHtml();

    AddStatusLine(wbuff);
    AddHeader(wbuff);
    AddContent(wbuff);
}

void HttpResponse::AddStatusLine(Buffer& buff){
    string status;
    if(CODE_STATUS.count(code_) == 1){
        // const 無法用[] Operator
        // status = CODE_STATUS[code_];
        status = CODE_STATUS.find(code_)->second;
    }
    // 请求出错
    else{
        code_ = 400;
        status = CODE_STATUS.find(400) -> second;
    }
    std::string line("HTTP/1.1 " + std::to_string(code_) + " " + status + "\r\n");
    buff.Append(line);
}

void HttpResponse::AddHeader(Buffer& buff){
    buff.Append("Connection: ");
    if(isKeepAlive_){
        buff.Append("keep-alive\r\n");
        buff.Append("keep-alive: max=6, timeout=120\r\n");
    }
    else{
        buff.Append("close\r\n");
    }
    buff.Append("Content-type: " + GetFileType() + "\r\n");
}

void HttpResponse::AddContent(Buffer& buff){
    int Fd = open((SrcDir_ + Path_).data(), O_RDONLY);
    if(Fd < 0){
        // 使用自定义的错误提示
        ErrorContent(buff, "File Error");
        return;
    }
    mmFile_ = (char*)mmap(0, mmFileStat_.st_size, PROT_READ, MAP_PRIVATE, Fd, 0);
    close(Fd);
    buff.Append("Content-length: " + std::to_string(mmFileStat_.st_size) + "\r\n\r\n");
}

void HttpResponse::UnmapFile(){
    // 解除mmap映射
    if(mmFile_) {
        munmap(mmFile_, mmFileStat_.st_size);
        mmFile_ = nullptr;
    }
}

void HttpResponse::ErrorHtml() {
    // 处理HTTP CODE对应错误
    if(CODE_PATH.count(code_) == 1) {
        Path_ = CODE_PATH.find(code_)->second;
        stat((SrcDir_ + Path_).data(), &mmFileStat_);
    }
}

string HttpResponse::GetFileType(){
    string::size_type idx = Path_.find_last_of('.');
    if(idx == string::npos){
        return "text/plain";
    }
    string suffix = Path_.substr(idx);
    if(MimeTypes.count(suffix) == 1){
        return MimeTypes.find(suffix)->second;
    }
    // 不支持的类型 使用默认类型 返回文本
    return "text/plain";
}

void HttpResponse::ErrorContent(Buffer& buff, string message){
    string body;
    string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if(CODE_STATUS.count(code_) == 1) {
        status = CODE_STATUS.find(code_)->second;
    } else {
        status = "Bad Request";
    }
    body += std::to_string(code_) + " : " + status  + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>TinyWebServer</em></body></html>";

    buff.Append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
    buff.Append(body);
}