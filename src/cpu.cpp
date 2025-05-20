#include "cpu.h"
#include "thread.h"
void interrupt(){
  thread::yield();
}
cpu::cpu(thread_startfunc_t func, uintptr_t arg){
  interrupt_vector_table[TIMER] = interrupt;
  interrupt_on();
  thread(func, arg);
}
void interrupt_on(){
  #ifdef INTERRUPT
  cpu::interrupt_enable();
  #endif
}
void interrupt_off(){
  #ifdef INTERRUPT
  cpu::interrupt_disable();
  #endif
}
