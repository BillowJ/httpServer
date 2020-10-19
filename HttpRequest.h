#ifndef HTTPREQUEST_H_
#define HTTPREQUEST_H_

#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <regex>

#include "Buffer.h"

namespace httpServer
{
    class HttpRequest{
     public:
        enum PARSE_STATE{
            Parse_Request_Line,
            Parse_Headers,
            Parse_Body,
            Parse_Finish,
        };
        
        enum HTTPMETHOD{
            GET,
            POST,
            INVALID
        };
        HttpRequest() { Init(); }
        ~HttpRequest() = default;

        void Init();
        bool Parse(Buffer&);
        bool IsKeepAlive() const;

        std::string Path() const { return Path_; }
        // 用于修改
        std::string& Path() { return Path_; }
        std::string HttpVersion() const { return HttpVesion_; }
        std::string Method() const { return Method_; }
        

     private:
        bool ParseRequestLine(const std::string&);
        void ParseHeaders(const std::string&);
        void ParseBody(const std::string&);
        void ParsePost();
        void ParsePath();

        int ConvertHex(char);
     private:
        using Map = std::unordered_map<std::string, std::string>;
        static const std::unordered_set<std::string> DEFAULT_HTML;
        // 当前解析状态
        PARSE_STATE ParseState_;
        std::string Path_, HttpVesion_, Method_, Body_;
        bool KeepAlive_;
        Map Header_;
        Map Post_;
    };
} // namespace httpServer

#endif