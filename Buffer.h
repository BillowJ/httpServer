#pragma once
#include <string>
#include <vector>
#include <atomic>
#include <cassert>

namespace httpServer
{
    class Buffer{
     public:
        static const size_t KInitalSize = 1024;

        explicit Buffer(size_t InitBufferSize = KInitalSize) 
        : buffer_(KInitalSize), 
          readIndex_(0),
          writeIndex_(0)
        {
            assert(ReadableBytes() == 0);
            assert(WritableBytes() == InitBufferSize);
            assert(PrepenableBytes() == 0);
        }

        ~Buffer() = default;

        size_t ReadableBytes() const;
        size_t WritableBytes() const;
        size_t PrepenableBytes() const;
        
        const char* Peek() const;

        void Retrieve(size_t);
        void RetrieveUntil(const char* end);
        void RetrieveAll();
        std::string RetrieveAllToStr();
        
        void EnsureWriteable(size_t len);
        void HasWritten(size_t len);

        void Append(const std::string& str);
        void Append(const char* str, size_t len);
        void Append(const void* data, size_t len);
        void Append(const Buffer& buff);

        ssize_t ReadFd(int fd, int* Errno);
        ssize_t WriteFd(int fd, int* Errno);

        char* BeginWrite()
        { return begin() + writeIndex_; }

        const char* ConstBeginWrite() const
        { return begin() + writeIndex_; }

     private:
        const char* begin() const 
        { return &*buffer_.begin(); }
        
        char* begin() 
        { return &*buffer_.begin(); }
     
        void MakeSpace(size_t len);

     private:
        std::vector<char> buffer_;
        size_t readIndex_;
        size_t writeIndex_;

    };


} // namespace httpServer
