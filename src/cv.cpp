#include "cv.h"
#include "cpu.h"
#include "thread.h"
#include <cassert>
cv::cv(){}
void cv::wait(mutex& mt){
  interrupt_off();
  ucontext_t* ucp = new ucontext_t();
  getcontext(ucp);
  waiting_queue.emplace_back(thread::running_tid, ucp);
  mt.unlock_wo_itr();
  if(!thread::running_queue().empty())
    thread::schedule(ucp);
  else 
  {
    std::cout << "All CPUs suspended.  Exiting.\n";
    exit(0);
  }
  mt.lock();
  interrupt_on();
}

void cv::broadcast_wo_itr(){
  for(auto t : waiting_queue){
    thread::running_queue().emplace_back(t);
  }
  waiting_queue.clear();
}

void cv::signal(){
  interrupt_off();
  if(waiting_queue.empty()) {
    interrupt_on();
    return;
  }
  thread::running_queue().emplace_back(waiting_queue.front());
  waiting_queue.pop_front();
  interrupt_on();

}
void cv::broadcast(){
  interrupt_off();
  broadcast_wo_itr();
  interrupt_on();
}
cv::~cv(){}
