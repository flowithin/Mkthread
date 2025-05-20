#include "thread.h"
#include "cpu.h"
#include "mutex.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <new>
#include <stdexcept>
#include <sys/types.h>
#include <sys/ucontext.h>
#include <system_error>
#include <ucontext.h>
#include <utility>
/*#define LOG*/
/*
 * Direct the new thread to use an allocated stack.  Your thread library
 * should allocate STACK_SIZE bytes for each thread's stack.  The initialization
 * of this stack will be done by makecontext.
 */



int thread::running_tid = 0;
std::unique_ptr<ucontext_t> thread::running_ucp(new ucontext_t());

void thread::join(){
  join_lock.lock();
  while(all_threads()[tid].state == State::running)
    thread::all_threads()[tid].cv_join->wait(join_lock);
  join_lock.unlock();
}



void thread_create(ucontext_t* ucontext_ptr, uintptr_t stub, uintptr_t func, uintptr_t arg, Stub* pass_stub){
/**/
  //this should just create a new context, a context for executing

  char* st_ptr_raw = new char [STACK_SIZE];
  ucontext_ptr->uc_stack.ss_sp = st_ptr_raw;
  ucontext_ptr->uc_stack.ss_size = STACK_SIZE;
  ucontext_ptr->uc_stack.ss_flags = 0;
  ucontext_ptr->uc_link = nullptr;
  pass_stub->st_ptr = std::unique_ptr<char[]>(st_ptr_raw);
  
  //only 3 args allowed

  makecontext(ucontext_ptr, reinterpret_cast<void (*)()>(stub), 3, func, arg, reinterpret_cast<uintptr_t>(pass_stub));
}



void stub(uintptr_t func, uintptr_t arg, uintptr_t pass_stub){
  //want to free this pointer after stub returns
  if(!thread::dealloc_queue().empty()) 
    thread::dealloc_queue().pop_front();
  std::unique_ptr<Stub> pass_stub_(reinterpret_cast<Stub*>(pass_stub));
  int tid_ = pass_stub_->tid;
  thread_startfunc_t func_ = reinterpret_cast<thread_startfunc_t>(func);
  interrupt_on();
  func_(arg);
  interrupt_off();
  //if the thread object has not been deleted
  if(thread::all_threads().find(tid_) != thread::all_threads().end()){
    thread::all_threads()[tid_].state = State::finished;
    thread::all_threads()[tid_].cv_join->broadcast_wo_itr();
  }
  //ready queue's threads should run
  thread::dealloc_queue().emplace_back(std::move(pass_stub_));
  if(!thread::running_queue().empty())
  {
    thread::schedule(nullptr);
  } else {
    std::cout << "All CPUs suspended.  Exiting.\n";
    exit(0);
  }
}


thread::thread(thread_startfunc_t func, uintptr_t arg)
/*
 * create a new context for stub, then fires, 
 * never return, which can be understood as
 * create a new thread and run
 * */
{
  interrupt_off();
  tid = thread::get_tid();
  ucontext_t* ucp = new ucontext_t();
  //tid + unique_ptr()
  Stub* pass_stub = new Stub(tid);
  try{
    thread_create(ucp, reinterpret_cast<uintptr_t>(stub), reinterpret_cast<uintptr_t>(func), arg, pass_stub);
  }
  catch(...){
    interrupt_on();
    throw(std::bad_alloc());
  }
  thread::all_threads().insert({tid, Join(State::running, new cv())});
  //if it is the first thread, run this new thread 
  if (tid == 0) setcontext(ucp);
  else thread::running_queue().emplace_back(tid, ucp);
  interrupt_on();
}

void thread::schedule(ucontext_t* ucp){
  //ucp can be nullptr
  //you must ensure running queue not empty at this time!
  running_ucp.swap(thread::running_queue().front().ucp);
  thread::running_tid = thread::running_queue().front().tid;
  thread::running_queue().pop_front();
  if(ucp == nullptr) setcontext(running_ucp.get());
  else if(swapcontext(ucp, running_ucp.get()) == -1)
    throw std::runtime_error("swapcontext failed");
  if(!thread::dealloc_queue().empty()) 
    thread::dealloc_queue().pop_front();
}


void thread::yield(){
interrupt_off();
  ucontext_t* ucp = new ucontext_t();
  getcontext(ucp);
  thread::running_queue().emplace_back(thread::running_tid, ucp);// NOTE: may lead to inefficiency, swapcontext will always create context?
  thread::schedule(ucp);
 
  interrupt_on();
}

thread::~thread(){
  //want to delete the cvs of this thread
  thread::all_threads().erase(tid);
}
