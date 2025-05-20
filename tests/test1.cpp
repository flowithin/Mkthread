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
/*cv cv1;*/

void loop(uintptr_t arg)
{
  /*for (int i=0; i < 100000; i++);*/
  /*std::cout << "here2\n";*/
  /*while(1){}*/
    auto id = reinterpret_cast<char*>(arg);
    mutex1.lock();
  /*std::cout << "here3\n";*/
  /*while(1){}*/
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
  thread t1 (loop, reinterpret_cast<uintptr_t>("child thread"));
  loop(reinterpret_cast<uintptr_t>("parent thread"));
}

int main()
{
    cpu::boot(1, parent, 0, false, false, RANDOMSEED);
}
