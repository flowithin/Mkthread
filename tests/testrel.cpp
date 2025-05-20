#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdexcept>
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
  mutex2.lock();
  mutex1.unlock();
  mutex2.unlock();
  try{
    mutex3.unlock();
  }
  catch(std::runtime_error&e){
    std::cout << "run time error" << '\n';
  }
  assert_interrupts_enabled();
}


void parent(uintptr_t arg)
{
  for(int i=0; i < tc; i++){
    int* x = new int(i);
     ta[i] = new thread (do_stuff, reinterpret_cast<uintptr_t>(x));
  assert_interrupts_enabled();
  }
  do_stuff(reinterpret_cast<uintptr_t>("parent thread"));
  for(int i=0; i < tc; i++){
    std::cout << "i = " << i << "\n";
    ta[i]->join();
  assert_interrupts_enabled();
  }
}
int main()
{
    cpu::boot(1, parent, 0, false, false, RANDOMSEED);
}
