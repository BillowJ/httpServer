#include "Utils.h"

void HandleSigpipe() {
  struct sigaction sa;
  memset(&sa, '\0', sizeof(sa));
  sa.sa_handler = SIG_IGN;
  sa.sa_flags = 0;
  sigaction(SIGPIPE, &sa, NULL);
  return;
}

bool SetNonBlocking(int fd){
    assert(fd > 0);
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
    return true;
}