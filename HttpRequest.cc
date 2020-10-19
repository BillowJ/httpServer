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
        /* Content\r\n
         * \r\n
         * BodyContent\r\n
         */
        const char* lineEnd = std::search(buff.Peek(), buff.ConstBeginWrite(), CRLF, CRLF+2);
        std::string line(buff.Peek(), lineEnd);
        switch (ParseState_)
        {
        case Parse_Request_Line:
            if(!ParseRequestLine(line)) return false;
            ParsePath();
            break;
        
        case Parse_Headers:
            ParseHeaders(line);
            if(buff.ReadableBytes() <= 2) { ParseState_ = Parse_Finish; }       // "\r\n"
            break;
        
        case Parse_Body:
            ParseBody(line);
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
        ParseState_ = Parse_Body;       // "\r\n"
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
int HttpRequest::ConvertHex(char ch) {
    if(ch >= 'A' && ch <= 'F') return ch -'A' + 10;
    if(ch >= 'a' && ch <= 'f') return ch -'a' + 10;
    return ch;
}
void HttpRequest::ParsePost(){
    int i = 0, j = 0;
    int size = Body_.size();
    std::string curKey("");
    std::string curVal("");
    int num = 0;
    for(; i < size; i++){
        char ch = Body_[i];
        // Encoding-Type: Percent-encoding -> https://developer.mozilla.org/zh-CN/docs/Glossary/percent-encoding
        switch(ch){
        case '=':
            curKey = Body_.substr(j, i - j);
            j = i + 1;
            break;
        case '&':
            curVal = Body_.substr(j, i - j);
            j = i + 1;
            Post_[curKey] = curVal;
            break;
        case '%':
            num = ConvertHex(Body_[i + 1]) * 16 + ConvertHex(Body_[i + 2]);
            Body_[i + 2] = num % 10 + '0';
            Body_[i + 1] = num / 10 + '0';
            i += 2;
            break;
        case '+':
            curVal = ' ';
        default:
            break;
        }
    }
    //last key-val
    assert(j <= i);
    if(Post_.count(curKey) == 0 && j < i) {
        curVal = Body_.substr(j, i - j);
        Post_[curKey] = curVal;
    }
}

void HttpRequest::ParseBody(const std::string& line){
    if(Method_ == "GET"){
        std::cout << "Method:GET\n";
        ParsePath();
    }
    else if(Method_ == "POST" && Header_["Content-Type"] == "application/x-www-form-urlencoded"){
        //...
        std::cout << "Method:POST\n";
        Body_ = line;
        ParsePost();
    }
    // TODO : parse application/json, multipart/form-data, text/xml 
    // else if(Method_ == "POST" && Header_["Content-Type"] == "text/plaint"){
    //     Body_ = line;
    // }
    
    // POST PARSE TEST:
    if(!Post_.empty()){
        for(auto& item : Post_){
            std::cout << "key: " << item.first;
            std::cout << " ";
            std::cout << "val: " << item.second;
            std::cout << std::endl;
        }
    }
    std::cout << "Parse Finish" << std::endl;
    ParseState_ = Parse_Finish;
}