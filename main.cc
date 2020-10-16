
#include "Server.h"
// #include "mem.h"

int main(){
    httpServer::Server* s = new httpServer::Server(8080, 6);
    s -> Start();
}