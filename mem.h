#pragma once
#include <iostream>
#include <exception>
#include <string>

using std::cout;
using std::endl;

//全局new重载
void *operator new(std::size_t size)
{
	cout << "全局new重载" << endl;
	void *mem = malloc(size);
	if (mem) return mem;
	else throw std::bad_alloc();
}

//全局new[]重载
void *operator new[](std::size_t size)
{
	cout << "全局new[]重载" << endl;
	void *mem = malloc(size);
	if (mem)
		return mem;
	else
		throw std::bad_alloc();
}

//全局delete重载
void operator delete(void *ptr)
{
	cout << "全局delete重载" << endl;
	if (ptr)
	{
		free(ptr);
	}
}

//全局delete[]重载
void operator delete[](void *ptr)
{
	cout << "全局delete[]重载" << endl;
	if (ptr)
	{
		free(ptr);
	}
}