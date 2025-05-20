#include <cstdint>
#include <iostream>
#include <stdexcept>
#include "cpu.h"
#include "thread.h"
#include "mutex.h"
#include "cv.h"
#define RANDOM_SEED 2
mutex one;
mutex two;
mutex three;
mutex four;
void loop(uintptr_t arg){
  one.lock();
  std::cout << "one locked\n";
  two.lock();
    auto id = reinterpret_cast<char*>(arg);
  std::cout << "loop called by " << id << std::endl;
  thread::yield();
}
void parent(uintptr_t arg)
{
  two.lock();
  one.lock();
    thread t1 (loop, reinterpret_cast<uintptr_t>("child thread"));
    loop(reinterpret_cast<uintptr_t>("parent thread"));
}
int main()
{
    cpu::boot(1, parent, 0, false, false, RANDOM_SEED);
}
