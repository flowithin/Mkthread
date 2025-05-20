# Kernel-Level Thread Library

This project implements a basic kernel-level thread library for use in educational or custom operating systems. It supports thread creation, context switching, scheduling (preemptive/cooperative), and essential synchronization primitives.

Designed for clarity, minimalism, and educational value in low-level threading concepts.

---

## Features

- Low-level thread creation and termination  
- Manual and automatic (preemptive) context switching  
- Cooperative and preemptive scheduling support  
- Basic synchronization primitives: mutexes, condition variables  
- Minimal external dependencies for integration in educational OS kernels  

---

## Core Components & Design Choices

### `ucontext_t` Layout (used for context management)

```cpp
ucontext_t *uc_link     // Context to resume when this one returns
sigset_t    uc_sigmask  // Signals blocked in this context
stack_t     uc_stack    // Stack for this context
mcontext_t  uc_mcontext // Machine-specific saved state (registers, PC, etc.)
```

---

### Thread Lifecycle

- Initialization: Create and initialize a new thread  
- Switching:
  - `getcontext`: Capture current context and prepare for swap
  - `setcontext`: Set a thread's context (used for first-time entry)
  - `swapcontext`: Save current context and switch to another  
- Sleeping / Blocking: Threads can block on locks or condition variables  
- Termination: Clean up and remove thread from system  

---

### Data Structures

#### In Thread Management:
- `running_queue`: `std::deque` holding ready-to-run thread contexts  
- `all_threads`: `std::unordered_set<tid, Join(state, cv)>`  
  Used for thread join management  

#### In Synchronization Primitives:
- `waiting_queue`: For threads blocked on mutexes or condition variables  

---

## Context Switching Flow

### 1. Thread Creation
- First thread: `setcontext` to jump directly into it  
- All others: `makecontext` → pushed into `ready_queue`  

### 2. Thread Yield / Switch
- Current thread pushed back to `ready_queue`  
- Scheduler pops next thread  
- `swapcontext` to resume next  

See: [swapcontext man page](https://linux.die.net/man/3/swapcontext)

---

## Signal Handling for Preemption

- Context switch on timer interrupt is treated like a normal function call  
- Interrupt handler creates a new stack frame above current user stack  
- When interrupt returns, user thread resumes execution naturally  

### Important Notes
- Disable interrupts before scheduling, re-enable after  
- Unlocking a mutex or calling `cv.wait()` may require disabling interrupts  

---

## Synchronization

Supports:
- Mutexes (locks)  
- Condition Variables  
- Semaphores (planned or minimal)  

---

## Safety & Correctness Checks

- All threads that aren't blocked should run  
- No memory leaks: all thread contexts are freed properly  
- FIFO order in ready queues  
- Threads should return normally  
- Interrupts are enabled when running user threads  
- Exit cleanly with no dangling locks or blocked threads  

---

## Error Handling

- Releasing a mutex not held → `std::runtime_error`  
- `std::bad_alloc` if stack memory can’t be allocated  
- `join` on invalid or already-joined thread  
- Robust condition variable memory handling  

---

## Test Coverage

- `test1`: Basic yield between threads  
- `test_join`: Multiple threads + join behavior  
- `testpiz`: Lock and condition variable correctness  
- `testrel`: Robust error handling  
- `testint`: Interrupt handling test  

---

## Future Improvements

- Proper semaphores  
- Thread priorities  
- Better memory pool for context allocation  
- Fine-grained timer tuning for preemption  
