/*
 * thread.h -- interface to the thread library
 *
 * This file should be included by the thread library and by application
 * programs that use the thread library.
 * 
 * You may add new variables and functions to this class.
 *
 * Do not modify any of the given function declarations.
 */

#pragma once
#include "cv.h"
#include "cpu.h"
#include "mutex.h"
#include <bits/types/stack_t.h>
#include <climits>
#include <iostream>
#include <cstdio>
#include <cstdint>
#include <deque>
#include <memory>
#include <sys/types.h>
#include <sys/ucontext.h>
#include <ucontext.h>
#include <unordered_map>
#include <cassert>
#include <utility>
#define INTERRUPT
/*#define LOG*/

#if !defined(__cplusplus) || __cplusplus < 201700L
#error Please configure your compiler to use C++17 or C++20
#endif

#if !defined(__clang__) && defined(__GNUC__) && __GNUC__ < 11
#error Please use g++ version 11 or higher
#endif

static constexpr unsigned int STACK_SIZE=262144;// size of each thread's stack in bytes
/*static constexpr unsigned int STACK_SIZE=UINT_MAX;// size of each thread's stack in bytes*/

using thread_startfunc_t = void (*)(uintptr_t);

struct TCB{
  int tid;
  std::unique_ptr<ucontext_t> ucp;
  TCB(int tid, ucontext_t* ucp):tid{tid},ucp{ucp}{}
  TCB(TCB& tcb):tid{tcb.tid},ucp{std::move(tcb.ucp)}{}
  ~TCB(){}
};
    enum class State{
      running,
      waiting,
      finished
    };
 
  struct Join{
    State state;
    std::unique_ptr<cv> cv_join;
  Join(State state, cv* cv):state{state}, cv_join{cv}{}
  Join():state{State::running}{}
  };
struct Stub{
  int tid;
  std::unique_ptr<char[]> st_ptr;
  Stub(int tid):tid{tid}{}
};
class thread {
  int tid;
  mutex join_lock;

  char* ss_init;
public:
   thread(thread_startfunc_t func, uintptr_t arg); // create a new thread
    ~thread();

    void join();                                // wait for this thread to finish

    static void yield();                        // yield the CPU

    /*
     * Disable the copy constructor and copy assignment operator.
     */
    thread(const thread&) = delete;
    thread& operator=(const thread&) = delete;

    /*
     * Move constructor and move assignment operator.  Implementing these is
     * optional in Project 2.
     */
    thread(thread&&);
    thread& operator=(thread&&);

    static std::deque<TCB>& running_queue() {
        static std::deque<TCB> deq;  // Static deque, persists throughout the program
        return deq;
    }
  static std::deque<std::unique_ptr<Stub>>& dealloc_queue() {
        static std::deque<std::unique_ptr<Stub>> deq;  // Static deque, persists throughout the program
        return deq;
    }
    static std::unordered_map<int, Join>& all_threads() {
        static std::unordered_map<int, Join> all_threads; //all the threads 
        return all_threads;
    }
  static int running_tid;
  static int get_tid(){
    static int tid_glob = 0;
    return tid_glob++;
  }
  int self_tid(){return tid;}
  char** get_ss_init(){return &ss_init;}
  static void schedule(ucontext_t* ucp);
  static std::unique_ptr<ucontext_t> running_ucp;
};
void stub(uintptr_t func, uintptr_t arg, uintptr_t self);
