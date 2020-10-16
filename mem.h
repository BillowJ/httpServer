#ifndef MEM_H
#define MEM_H

#include "Mmalloc.h"
#include <iostream>
#include <exception>
#include <string>

using std::cout;
using std::endl;
using namespace global;
void *operator new(std::size_t size)
{
	void *mem = tcmalloc::Mmalloc(size);
	if(mem) return mem;
    else return (void*)(0);
}

//全局new[]重载
void *operator new[](std::size_t size)
{
	return operator new(size);
}

//全局delete重载
void operator delete(void *ptr)
{
	if (ptr)
	{
		tcmalloc::Mfree(ptr);
	}
}

//全局delete[]重载
void operator delete[](void *ptr)
{
	if (ptr)
	{
		operator delete(ptr);
	}
}

#endif