
#include "Server.h"

int main(){
    httpServer::Server s(8081, 5);
    s.Start();
}