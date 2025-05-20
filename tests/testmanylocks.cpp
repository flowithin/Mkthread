#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "cpu.h"
#include "thread.h"
#include "mutex.h"
#include "cv.h"
#define RANDOM_SEED 2
std::vector<int> vint;
  cv loop_cv;
void loop(uintptr_t arg){
  mutex cv_lock;
  cv_lock.lock();
  while (vint.size() < 10) {
    loop_cv.wait(cv_lock);
  }
  auto id = reinterpret_cast<char*>(arg);
  std::cout << "loop called by " << id << std::endl;
  cv_lock.unlock();
}
void parent(uintptr_t arg)
{
  for(int i=0; i < 10; ++i){
    //test mutex deletion 
    mutex m;
    m.lock();
    vint.push_back(i);
    thread t1 (loop, reinterpret_cast<uintptr_t>("child thread"));
    m.unlock();
    /*loop(reinterpret_cast<uintptr_t>("parent thread"));*/
    std::cout << "parent now broadcase\n";
    loop_cv.broadcast();
  }
}
void init(uintptr_t arg){
  thread parentt(parent, reinterpret_cast<uintptr_t>("parent"));
}
int main()
{
    cpu::boot(1, init, 0, false, false, RANDOM_SEED);
}
