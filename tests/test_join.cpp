#include <cstdint>
#include <cstdio>
#include <iostream>
#include <string>
#include <cstring>
#include "cpu.h"
#include "thread.h"
#include "mutex.h"
#include "cv.h"
#define RANDOMSEED 1000
using std::cout;
using std::endl;
const int tc=20;

char str[100];
thread* ta[tc];
thread* pthr;
int x=0;
void loop(uintptr_t arg)
{
    auto id = reinterpret_cast<int*>(arg);
    cout << "loop called by " << *id << endl;
  thread::yield();
}
void parent(uintptr_t arg)
{
  for(int i=0; i < tc; i++){
    int* x = new int(i);
     ta[i] = new thread (loop, reinterpret_cast<uintptr_t>(x));
      /*std::cout << ta[i]->self_tid() << '\n';*/
  }
  loop(reinterpret_cast<uintptr_t>("parent thread"));
  for(int i=0; i < tc; i++){
    std::cout << "i = " << i  << "\n";
    ta[i]->join();
  }
}

int main()
{
    cpu::boot(1, parent, 0, false, false, RANDOMSEED);
}
