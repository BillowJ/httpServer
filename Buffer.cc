#include "Buffer.h"
#include <sys/uio.h> //readv
#include <unistd.h>

using namespace httpServer;


size_t Buffer::ReadableBytes() const{
    return writeIndex_ - readIndex_;
}

size_t Buffer::WritableBytes() const{
    return buffer_.size() - writeIndex_;
}

size_t Buffer::PrepenableBytes() const{
    return readIndex_;
}

const char*  Buffer::Peek() const {
    return begin() + readIndex_;
}

void Buffer::RetrieveAll(){
    // bzero(&buffer_[0], buffer_.size());
    readIndex_ = 0;
    writeIndex_ = 0;
}

void Buffer::Retrieve(size_t len){
    assert(len <= ReadableBytes());
    if(len < ReadableBytes()){ readIndex_ += len; }
    else{ RetrieveAll(); }
}

std::string Buffer::RetrieveAllToStr(){
    size_t len = ReadableBytes();
    std::string str(Peek(), len);
    RetrieveAll();
    return str;
}

// XXX : 
void Buffer::RetrieveUntil(const char* end){
    assert(Peek() <= end );
    Retrieve(end - Peek());
}

void Append(const std::string& str);
void Append(const char* str, size_t len);
void Append(const void* data, size_t len);
void Append(const Buffer& buff);

void Buffer::MakeSpace(size_t len){
    if(len > (WritableBytes() + PrepenableBytes())){
        buffer_.resize(writeIndex_ + len + 1);
    }
    else
    {
        size_t readSize = ReadableBytes();
        std::copy(begin() + readIndex_, begin() + writeIndex_, begin());
        readIndex_ = 0;
        writeIndex_ = readIndex_ + readSize;
        assert(readSize == ReadableBytes());
    }
    
}

void Buffer::HasWritten(size_t len){
    assert(len <= WritableBytes());
    writeIndex_ += len;
}
void Buffer::EnsureWriteable(size_t len){
    if (len > WritableBytes())
    {
        MakeSpace(len);
    }
    assert(len <= WritableBytes());
}

void Buffer::Append(const std::string& str) {
    Append(str.data(), str.length());
}

void Buffer::Append(const void* data, size_t len) {
    assert(data);
    Append(static_cast<const char*>(data), len);
}

void Buffer::Append(const char* str, size_t len) {
    assert(str);
    EnsureWriteable(len);
    std::copy(str, str + len, BeginWrite());
    HasWritten(len);
}

void Buffer::Append(const Buffer& buff) {
    Append(buff.Peek(), buff.ReadableBytes());
}


ssize_t Buffer::ReadFd(int fd, int* saveErrno) {
    /* 开辟的栈空间，128k */
    char buff[65535];
    struct iovec iov[2];
    const size_t writable = WritableBytes();
    /* 分散读， 保证数据全部读完 */
    iov[0].iov_base = begin() + writeIndex_;
    iov[0].iov_len = writable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    const ssize_t len = readv(fd, iov, 2);
    if(len < 0) {
        *saveErrno = errno;
    }
    else if(static_cast<size_t>(len) <= writable) {
        writeIndex_ += len;
    }
    else {
        writeIndex_ = buffer_.size();
        Append(buff, len - writable);
    }
    return len;
}

ssize_t Buffer::WriteFd(int fd, int* Errno_) {
    size_t readSize = ReadableBytes();
    ssize_t len = write(fd, Peek(), readSize);
    if(len < 0) {
        *Errno_ = errno;
        return len;
    }
    readIndex_ += len;
    return len;
}