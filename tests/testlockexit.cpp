#include <cstdint>
#include <iostream>
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

mutex mutex1;
mutex mutex2;
mutex mutex3;
mutex mutex4;
void do_stuff(uintptr_t arg){
  mutex1.lock();
  std::cout<< "m1 lock\n";
  mutex2.lock();
  std::cout << "m2 lock\n";
  mutex3.lock();
  thread::yield();
  mutex3.unlock();
  thread::yield();
  mutex4.lock();
  thread::yield();
  mutex2.unlock();
  thread::yield();
  mutex4.unlock();
  thread::yield();
  mutex1.unlock();
  thread::yield();
}


void parent(uintptr_t arg)
{
  for(int i=0; i < tc; i++){
    int* x = new int(i);
     ta[i] = new thread (do_stuff, reinterpret_cast<uintptr_t>(x));
      /*std::cout << ta[i]->self_tid() << '\n';*/
  }
  do_stuff(reinterpret_cast<uintptr_t>("parent thread"));
  for(int i=0; i < tc; i++){
    std::cout << "i = " << i << "\n";
  thread::yield();
    ta[i]->join();
  }
}
int main()
{
    cpu::boot(1, parent, 0, false, false, RANDOMSEED);
}
