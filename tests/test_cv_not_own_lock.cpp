#include <cstdint>
#include <iostream>
#include <stdexcept>
#include "cpu.h"
#include "thread.h"
#include "mutex.h"
#include "cv.h"
#define RANDOM_SEED 2
thread* tf;
cv cv;
mutex cvlock;
mutex lock;
void loop(uintptr_t arg){
  auto id = reinterpret_cast<char*>(arg);
  std::cout << "loop called by " << id << std::endl;
  cvlock.lock();
  thread::yield();
  std::cout << "before wait\n";
  cv.wait(lock);
  std::cout << "after wait\n";
  cvlock.unlock();
  std::cout << "end\n";
}
void parent(uintptr_t arg)
{
  for(int i=0; i < 10; i++)
  thread t1(loop, reinterpret_cast<uintptr_t>("child thread"));
}

int main()
{
  cpu::boot(1, parent, 0, false, false, RANDOM_SEED);
}
