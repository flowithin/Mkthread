#include "cpu.h"
#include "thread.h"
#include <cstdio>
#include <stdexcept>
#include "mutex.h"
mutex::mutex():guard{0}, owner{-1}{}


void mutex::lock_wo_itr(){
  if(guard == 0){guard = 1;}
  else{
    ucontext_t* ucp = new ucontext_t();
    getcontext(ucp);
    waiting_queue.emplace_back(thread::running_tid, ucp);
    if (!thread::running_queue().empty())
      thread::schedule(ucp);
    else {
      std::cout << "All CPUs suspended.  Exiting.\n";
      exit(0);
    }
  }
  owner = thread::running_tid;
}



void mutex::lock(){
  interrupt_off();
  lock_wo_itr();
  interrupt_on();
}
void mutex::unlock(){
  interrupt_off();
  unlock_wo_itr();
  interrupt_on();
}
void mutex::unlock_wo_itr(){
  if(owner != thread::running_tid)
  {
    interrupt_on();
    throw (std::runtime_error("release not owned lock"));
  }
  guard = 0;
  owner = -1;
  if (!waiting_queue.empty())
  {
    thread::running_queue().emplace_back(waiting_queue.front());
    waiting_queue.pop_front();
    guard = 1;
    owner = thread::running_queue().back().tid;
  }
}
