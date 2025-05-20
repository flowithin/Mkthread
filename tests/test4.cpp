#include <iostream>
#include "cpu.h"
#include "thread.h"
#include "mutex.h"
#include "cv.h"
#define RANDOMSEED 1000
using std::cout;
using std::endl;


int g = 0;

mutex mutex1;

// Unit Test: Thread Lifetime
void loop(uintptr_t arg)
{
  auto id = reinterpret_cast<char*>(arg);
  mutex1.lock();
  cout << "loop called by " << id << endl;
  int i;
  for (i=0; i<10; i++, g++) {
      cout << id << ":	" << i << "	" << g << endl;
      mutex1.unlock();
      thread::yield();
      mutex1.lock();
  }
  cout << id << ":	" << i << "	" << g << endl;
  mutex1.unlock();
}

void parent(uintptr_t arg)
{
  {
    thread t1 (loop, reinterpret_cast<uintptr_t>("child thread"));
  }
  // thread goes out of scope
  loop(reinterpret_cast<uintptr_t>("parent thread"));
}

int main()
{
    cpu::boot(1, parent, 0, false, false, RANDOMSEED);
}
