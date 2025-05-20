# Kernel-Level Thread Library

This project implements a basic kernel-level thread library that provides foundational threading functionality within a custom or educational operating system kernel. It focuses on lightweight thread creation, context switching, and cooperative or preemptive scheduling.

## Features

- Low-level thread creation and termination
- Context switching via stack and register management
- Preemptive and/or cooperative scheduling support
- Basic synchronization primitives (e.g., locks, condition variables)
- Minimal dependencies for educational OS environments

## *Some design choices
```cpp

ucontext_t *uc_link     //pointer to the context that will be resumed
	                       //when this context returns
sigset_t    uc_sigmask  //the set of signals that are blocked when this
                        //context is active
stack_t     uc_stack    //the stack used by this context
mcontext_t  uc_mcontext //a machine-specific representation of the saved
                        //context
```
#### threads management
- init(create) threads
- switch threads
	- getcontext(context_t* ucp)
		- just to push the newly created pointer to ready queue 
		- then would use swapcontext to do the swap
	- setcontext
	- swapcontext
- threads sleep
- threads lock(block)
- threads kill

#### data structures:
- in **thread**:
	- running_queue: **deques** of ucps (ready queues, waiting queues) (heap, static)
	- all_threads: a ~~hashmap~~ **hashset** < tid , Join(state, cv)> (for join) (heap, static)
- in cv/lock
	- waiting_queue: 
#### Context switching

1. thread creation: 
	1. other threads: makecontext -> push into ready queue.
	2. if the first thread: setcontext
2. thread change (yield): push to ready queue -> pop from ready queue -> swap context 
https://linux.die.net/man/3/swapcontext
#### signal handling
- The way PC values, register values, etc are treated is the same as if you *called a function*. The only difference is that this function may be called at an arbitrary point in time that the user can not control.
- when an interrupt happens, it will just be a stack frame up the user thread. when it returns, it naturally returns to the user thread
![[Pasted image 20250130224550.png]]

- how do we extract the interrupted threads' info?
	- no need to do so, just return then it returns
- remember to **add interrupt after** schedule and **diable before** 
- does `unlock` and `cv.wait` really need to turn off interrupt? 

#### Mutex, cv, semaphores

#### safety checks
- no memory leaks
	- all the contexts are freed
- threads should be running if they are not locked
- FIFO order dequeue
- threads should return normally
- interrupt **enabled** running usr prog
- exit properly, mutex, cv, stub (testpiz)
- error handling (testrel)
- lock, cv functions (testpiz)
- join and many threads(test_join)
- yield (test1)
- interrupt ('all' ? )
#### Error handling
- release mutex not held `std::runtime_error`
- `std::bad_alloc`, not enough stack space
- thread join
	- cv may take too much memory
	- where does the cv live?

