#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cassert>
#include <memory>
#include <sys/types.h>
#include <vector>
#include "cv.h"
#include "cpu.h"
#include "thread.h"

cv pp;
mutex ppm;
int pingc=1, pongc=0;
void ping(uintptr_t arg){
  while (pingc < 20) {
    thread::yield();
   ppm.lock();
  while(pongc - pingc <= 0){
    pp.wait(ppm);
  }
  std::cout << "ping\n";
  pingc+=2;
  pp.signal();
  ppm.unlock();
  }
}
void pong(uintptr_t arg){
  while (pongc < 20) {
    thread::yield();
   ppm.lock();
  while(pongc-pingc >= 0){
    pp.wait(ppm);
  }
  std::cout << "pong\n";
  pongc+=2;
  pp.signal();
  ppm.unlock();
 
  }
}
void ancestor(uintptr_t arg){
  thread(ping,uintptr_t());
  thread(pong,uintptr_t());
  }

int main(){
  cpu::boot(1, ancestor, uintptr_t(),false, false, 0);
}
