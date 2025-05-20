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
void do_stuff(uintptr_t arg){
  mutex1.lock();
  std::cout << "locked!\n";
  mutex1.unlock();
  std::cout << "m1 unlocked!\n";
  mutex1.unlock();
  std::cout << "m1 unlocked again!\n";
}


void parent(uintptr_t arg)
{
  for(int i=0; i < tc; i++){
    int* x = new int(i);
     ta[i] = new thread (do_stuff, reinterpret_cast<uintptr_t>(x));
      /*std::cout << ta[i]->self_tid() << '\n';*/
  }
  do_stuff(reinterpret_cast<uintptr_t>("parent thread"));
}
int main()
{
    cpu::boot(1, parent, 0, false, false, RANDOMSEED);
}
