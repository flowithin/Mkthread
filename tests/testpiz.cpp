#include <cassert>
#include <climits>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <sstream>
#include <vector>
#include "cv.h"
#include "cpu.h"
#include "thread.h"


mutex cout_mutex;
struct location_t {
    unsigned int x;
    unsigned int y;
};
std::ostream& operator<<(std::ostream& os, const location_t& loc) {
        os << "(" << loc.x << ", " << loc.y << ")";
        return os;
    }

void driver_ready(unsigned int driver, location_t location){
  cout_mutex.lock();
  std::cout << "driver " << driver << "ready at " << location << '\n';
  cout_mutex.unlock();
}
void drive(unsigned int driver, location_t start, location_t end){
  cout_mutex.lock();
  std::cout << "driver " << driver << "driving from " << start << " to " << end << '\n';
  cout_mutex.unlock();
}

void customer_ready(unsigned int customer, location_t location){
cout_mutex.lock();
  std::cout << "customer " << customer << "ready at " << location << '\n';
  cout_mutex.unlock();

}
void pay(unsigned int customer, unsigned int driver){
cout_mutex.lock();
  std::cout << "customer " << customer << " pays " << driver << '\n';
  cout_mutex.unlock();


}

void match(unsigned int customer, unsigned int driver){
cout_mutex.lock();
  std::cout << "customer " << customer << " matches with " << driver << '\n';
  cout_mutex.unlock();


}

cv cv_fd,cv_driver,cv_customer;
mutex vec_lock;
bool fc = false;
//these stands for the waiting customer/driver, active driver/customer, the request count in running/total
int wd=0, wc = 0, ad=0, rqc = 0, rqct = 0;
thread* tf;
struct Person{
  location_t loc;
  bool ready, served;
  unsigned int cid, did;
  Person(location_t& loc, bool ready, bool served, unsigned int cid, unsigned int did):loc{loc},ready{ready}, served{served}, cid{cid},did{did}{}
};
struct customer_arg{
  unsigned int cid;
  std::vector<location_t> loc;
};
struct driver_arg{
  unsigned int did;
  location_t loc;
};
struct Ans{
  int argc;
  char** argv;
};
std::vector<Person> dvec, cvec;
//given a driver id, matched its customer (closest one)
bool try_match(unsigned int did){
  int min_dist = INT_MAX;
  unsigned int cid;
  for(auto c : cvec){
    if (!c.ready) continue;
    int dist = abs(static_cast<int>(c.loc.x) -  static_cast<int>(dvec[did].loc.x)) + abs(static_cast<int>(c.loc.y) - static_cast<int>(dvec[did].loc.y));
    if(dist < min_dist){
      min_dist = dist;
      cid = c.cid;
    } 
  }
  if(min_dist == INT_MAX || !dvec[did].ready) return false;
  cvec[cid].ready = dvec[did].ready = false;
  cvec[cid].did = did;
  dvec[did].cid = cid;
  match(cid, did);
  wc--;
  return true;
}
//customer thread function
void customer(uintptr_t arg){
  customer_arg* captr_ = reinterpret_cast<customer_arg*>(arg);
  std::unique_ptr<customer_arg> captr(captr_);
  tf->join();
  for(int i = 0; i < static_cast<int>(captr->loc.size()); i++){
    vec_lock.lock();
    std::cout << "locked by customer" << captr->cid << '\n';
    (cvec[captr->cid].loc = captr->loc[i], cvec[captr->cid].ready = true, cvec[captr->cid].served = false);
    customer_ready(captr->cid, captr->loc[i]);
    wc++;
    //wait for driver to match
    cv_driver.signal();
    assert_interrupts_enabled();
    while(!cvec[captr->cid].served){
      cv_customer.wait(vec_lock); 
    }
    unsigned int did = cvec[captr->cid].did;
    pay(captr->cid, did);
    assert(did < dvec.size());
    dvec[did].served = true;
    rqc++;
    //paid driver, broadcast to ensure it get notified
    cv_driver.broadcast();
    /*std::cout<<"after broadcast\n";*/
    vec_lock.unlock();
    /*std::cout<<"after unlock\n";*/
  }
}
//driver thread function
void driver(uintptr_t arg){
  driver_arg* daptr_ = reinterpret_cast<driver_arg*>(arg);
  std::unique_ptr<driver_arg> daptr(daptr_);
  tf->join();
  while(1){
    vec_lock.lock();
    std::cout << "locked by driver" << daptr->did << '\n';
    //if all requests handled, get ready
    while((ad > 0 || wc <= 0) && (rqc != rqct)){
      wd++;
      cv_driver.wait(vec_lock);
      wd--;
    }
    ad++;
    location_t curr_pos = dvec[daptr->did].loc;
    driver_ready(daptr->did, curr_pos);
    dvec[daptr->did].ready = true;
    //if can't match, wait on finished drivers queue
    while (!try_match(daptr->did)) {cv_fd.wait(vec_lock);}
    location_t  dest = cvec[dvec[daptr->did].cid].loc, from = dvec[daptr->did].loc;
    ad--;
    //pick another driver to ready
    cv_driver.signal();
    vec_lock.unlock();
    drive(daptr->did, from, dest);
    vec_lock.lock();
    std::cout << "locked by driver" << daptr->did << '\n';
    dvec[daptr->did].loc = dest;
    dvec[daptr->did].served = false;
    unsigned int cid = dvec[daptr->did].cid;
    cvec[cid].served = true;
    //finished delivery, wait for payment
    cv_customer.broadcast();
    while(!dvec[daptr->did].served){
      cv_driver.wait(vec_lock);
    }
    vec_lock.unlock();
  }
}
//this is the main program that spawns all the customers and drivers
void init(uintptr_t arg){
  vec_lock.lock();
  Ans* ans_ = reinterpret_cast<Ans*>(arg);
  std::unique_ptr<Ans> ans(ans_);
  char** files = ans->argv;
  //spawning drivers
  for(int i = 0; i < 30; i++){
    driver_arg* draptr = new(driver_arg);
    (draptr->loc = {0,0},draptr->did = static_cast<unsigned int>(i));
    dvec.push_back({draptr->loc, false, false, UINT_MAX, draptr->did});
    thread(driver, reinterpret_cast<uintptr_t>(draptr));
  }
  unsigned int x, y;
  //spawning customers
  for(int cnt = 0; cnt < ans->argc-2; cnt++){
    /*const char* infilename = files[cnt+2];*/
    /*std::fstream cfs(infilename, std::ios::in);*/
    std::string input = "10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n";  // Input data
    std::stringstream ss(input);
    customer_arg* cargptr = new(customer_arg);
    cargptr->cid = cnt;
    std::vector<location_t> loc_vec;
    while(ss >> x >> y){
      loc_vec.push_back(location_t{x,y});
      /*std::cout<< x <<y << '\n';*/
      rqct++;//count total customer
    }
    cvec.push_back({(cargptr->loc = loc_vec)[0], false, false, static_cast<unsigned int>(cnt), UINT_MAX});
    thread(customer, reinterpret_cast<uintptr_t>(cargptr));
  }
  vec_lock.unlock();
}
void ancestor(uintptr_t arg){
  tf = new thread(init, arg);
}
int main(){
  Ans* ans = new(Ans);
    ans->argv = nullptr;
  ans->argc = 20;
  uintptr_t argv_ = reinterpret_cast<uintptr_t>(ans);
  cpu::boot(1, ancestor, argv_,false, false, 0);
}

