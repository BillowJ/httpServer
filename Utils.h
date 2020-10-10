#include <fcntl.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

bool SetNonBlocking(int fd){
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
    return true;
}

void HandleSigpipe() {
  struct sigaction sa;
  memset(&sa, '\0', sizeof(sa));
  sa.sa_handler = SIG_IGN;
  sa.sa_flags = 0;
  if (sigaction(SIGPIPE, &sa, NULL)) return;
}