
#include "Server.h"
// #include "mem.h"

void func(void* arg){
    string* val = (string*)arg;
    std::cout << *val;
}

int main(){
    httpServer::Server* s = new httpServer::Server(8080, 6);
    s ->ResigterHandler("sad", func);
    s -> Start();
}