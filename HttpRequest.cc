#include "HttpRequest.h"

using namespace httpServer;

const std::unordered_set<std::string> HttpRequest::DEFAULT_HTML{
            "/index", 
            "/hah",  };


void HttpRequest::Init(){
    Path_ = HttpVesion_ = Method_ = "";
    ParseState_ = Parse_Request_Line;
    Header_.clear();
    Body_.clear();
}

bool HttpRequest::IsKeepAlive() const{
    auto res = Header_.find("Connection");
    if(res != Header_.end()){
        return res->second == "keep-alive" && HttpVesion_ == "1.1";
    }
    return false;
}

bool HttpRequest::Parse(Buffer& buff){
    const char* CRLF = "\r\n";
    if(buff.ReadableBytes() <= 0){
        return false;
    }
    while (buff.ReadableBytes() && ParseState_ != Parse_Finish)
    {   
        const char* lineEnd = std::search(buff.Peek(), buff.ConstBeginWrite(), CRLF, CRLF+2);
        std::string line(buff.Peek(), lineEnd);
        switch (ParseState_)
        {
        case Parse_Request_Line:
            ParseRequestLine(line);
            break;
        
        case Parse_Headers:
            ParseHeaders(line);
            break;
        
        case Parse_Body:
            ParseBody();
            break;
        default:
            break;
        }
    }
    // success
    return true;
}

bool HttpRequest::ParseRequestLine(const std::string& str){
    bool succed = false;
    std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch subMatch;
    if(std::regex_match(str, subMatch, patten)) {
        Method_ = subMatch[1];
        Path_ = subMatch[2];
        HttpVesion_ = subMatch[3];
        ParseState_ = Parse_Headers;
        succed = true;
    }
    return succed;
}

void HttpRequest::ParseHeaders(const std::string& str){
    std::regex patten("^([^:]*): ?(.*)$");
    std::smatch subMatch;
    if(std::regex_match(str, subMatch, patten)) {
        Header_[subMatch[1]] = subMatch[2];
    }
    else
    {
        ParseState_ = Parse_Body;
    }
    return;
}

void HttpRequest::ParsePath(){
    if(Path_ == "/"){
        Path_ = "/index.html";
    }
    else{
        for(auto& item : DEFAULT_HTML){
            if(item == Path_){
                Path_ = Path_ + ".html";
                return;
            }
        }
    }
}

void HttpRequest::ParseBody(){
    if(Method_ == "GET"){
        ParsePath();
        ParseState_ = Parse_Finish;
    }
    else if(Method_ == "POST"){
        //...
    }
    ParseState_ = Parse_Finish;
}