#include <cstdint>
#include <iostream>
#include <stdexcept>
#include "cpu.h"
#include "thread.h"
#include "mutex.h"
#include "cv.h"
#define RANDOM_SEED 2
thread* tp;
void loop(uintptr_t arg){
    auto id = reinterpret_cast<char*>(arg);
  std::cout << "loop called by " << id << std::endl;
  tp->join();
}
void parent(uintptr_t arg)
{
    tp = new thread (loop, reinterpret_cast<uintptr_t>("child thread"));
  thread::yield();
    loop(reinterpret_cast<uintptr_t>("parent thread"));
}
int main()
{
    cpu::boot(1, parent, 0, false, false, RANDOM_SEED);
}
