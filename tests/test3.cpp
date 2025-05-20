#include <iostream>
#include "cpu.h"
#include "thread.h"
#include "mutex.h"
#include "cv.h"
#define RANDOMSEED 1000
using std::cout;
using std::endl;


int g = 0;

// Unit Test: Yielding at the presence of only one thread
void loop(uintptr_t arg)
{
  auto id = reinterpret_cast<char*>(arg);
  cout << "loop called by " << id << endl;
  int i;
  for (i=0; i<10; i++, g++) {
      cout << id << ":	" << i << "	" << g << endl;
      thread::yield();
  }
  cout << id << ":	" << i << "	" << g << endl;
}

void parent(uintptr_t arg)
{
  loop(reinterpret_cast<uintptr_t>("parent thread"));
}

int main()
{
    cpu::boot(1, parent, 0, false, false, RANDOMSEED);
}
