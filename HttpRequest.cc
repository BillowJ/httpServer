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
            if(buff.ReadableBytes() <= 2) { ParseState_ = Parse_Finish; }
            break;
        default:
            break;
        }
    }
    // success
    return true;
}

void HttpRequest::ParseRequestLineWithParameter(std::string str){
    std::string::size_type n = str.find("?");
    // GET without parameter
    if(n == std::string::npos){
        Path_ = str;
        return;
    }
    else
    {
        Path_ = str.substr(0, n);
        
        size_t i = n;
        size_t j = n;
        int num = 0;
        size_t curSize = str.size();
        std::string curKey("");
        std::string curVal("");
        //Decode ParseLine.
        for(size_t i = n, j = n; i < curSize; i++){
            char ch = str[i];
            switch (ch)
            {
            case '=':
                curKey = str.substr(j, i-j);
                j = i + 1;
                break;

            case '&':
                curVal = str.substr(j, i-j);
                j = i+1;
                if(curVal == "") curVal = "default";
                Keys_[curKey] = curVal;
                break;

            case '%':
                // 将16进制转换成10进制的ASCII码,到时候注册的时候只需匹配相应的ASCII码确定Key
                num = ConvertHex(str[i + 1]) * 16 + ConvertHex(str[i + 2]);
                str[i + 2] = num % 10 + '0';
                str[i + 1] = num / 10 + '0';
                i += 2;
                break;

            case '+':
                curVal = ' ';
                break;

            default:
                break;
            }
        }
        assert(j <= i);
        if(j <= i && Keys_.count(curKey) == 0){
            curVal = str.substr(j, i - j);
            if(curVal == "") curVal = "default";
            Keys_[curKey] = curVal;
        }
    }
    
}

bool HttpRequest::ParseRequestLine(const std::string& str){
    bool succed = false;
    std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch subMatch;
    if(std::regex_match(str, subMatch, patten)) {
        Method_ = subMatch[1];
        ParseRequestLineWithParameter(subMatch[2]);
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


void HttpRequest::ParseBody(const std::string& line){
    if(line.empty()) ParseState_ = Parse_Finish;

    if(Method_ == "GET"){
        std::cout << "Method:GET\n";
        ParsePath();
    }
    else if(Method_ == "POST" && Header_["Content-Type"] == "application/x-www-form-urlencoded"){
        std::cout << "Method:POST\n";
        // 仅保存Body的数据
        Body_ += line;
    }

    std::cout << "Parse Finish" << std::endl;
    std::cout << "Path_: " << Path_ << std::endl;
    ParseState_ = Parse_Finish;
}


// POST 交给业务层提供函数进行解析 不进行自我解析
/*
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
*/