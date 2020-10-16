#ifndef UTILS_H_
#define UTILS_H_

#include <fcntl.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

bool SetNonBlocking(int fd);

void HandleSigpipe();

#endif