/*
 * mutex.h -- interface to the mutex class
 *
 * You may add new variables and functions to this class.
 *
 * Do not modify any of the given function declarations.
 */

#pragma once

#include <ucontext.h>
#include "cv.h"
#include "mutex.h"
#include <deque>

class mutex {
  std::deque<struct TCB> waiting_queue;
  int guard;
  int owner;
  void lock_wo_itr();
  void unlock_wo_itr();
public:
    mutex();
    ~mutex(){};

    void lock();
    void unlock();

    /*
     * Disable the copy constructor and copy assignment operator.
     */
    mutex(const mutex&) = delete;
    mutex& operator=(const mutex&) = delete;

    /*
     * Move constructor and move assignment operator.  Implementing these is
     * optional in Project 2.
     */
    mutex(mutex&&);
    mutex& operator=(mutex&&);
  friend class cv;
};


//weidly this doesn't work
/*void wait(std::deque<struct TCB>& waiting_queue){*/
/*  ucontext_t* ucp = new ucontext_t();*/
/*  getcontext(ucp);*/
/*  waiting_queue.emplace_back(thread::running_tid, ucp);*/
/*  thread::schedule(ucp);*/
/*}*/
