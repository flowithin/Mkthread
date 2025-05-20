#include <algorithm>
#include <sstream>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cassert>
#include <memory>
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
cv cv_driver;
cv cv_customer;
mutex cv_lock, cvec_lock, dvec_lock, Driver_lock, Custormer_lock,match_lock;
int wait_driver,wait_customer,wait_customer_rc,matching_drive;
struct Driver{
  location_t loc;
  bool free,paid;
  unsigned int cid;
  unsigned int did;
  Driver(location_t& loc, bool free, bool paid, unsigned int cid, unsigned int did):loc{loc},free{free}, paid{paid}, cid{cid},did{did}{}
};
struct Customer{
  location_t loc;
  bool requesting, received;
  unsigned int did;
  unsigned int cid;
  Customer(location_t loc, bool req, bool rec, unsigned int did, unsigned int cid):loc{loc},requesting{req}, received{rec}, did{did},cid{cid}{}
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
std::vector<Customer> cvec;
std::vector<Driver> dvec;


bool try_match(unsigned int did){
  /*cout_mutex.lock();*/
  /*std::cout << "trymatching\n";*/
  /*cout_mutex.unlock();*/
  int min_dist = INT_MAX;
  unsigned int cid;
  int n_c_dist;
  for(auto c : cvec){
    if (!c.requesting) continue;
    int dist = abs(static_cast<int>(c.loc.x ) -  static_cast<int>(dvec[did].loc.x)) + abs(static_cast<int>(c.loc.y) - static_cast<int>(dvec[did].loc.y));
    if(dist < min_dist){
      /*std::cout << "dist = " << dist << '\n';*/
      min_dist = dist;
      cid = c.cid;
      /*std::cout << "cid = " << cid << '\n';*/
    } 
  }
  if(min_dist == INT_MAX) return false;
  n_c_dist = min_dist;
  min_dist = INT_MAX;
   for(auto d : dvec){
    /*std::cout<<"here\n";*/
    if(!d.free) continue;
    /*std::cout<<"here\n";*/
    int dist = abs(static_cast<int>(d.loc.x) - static_cast<int>(cvec[cid].loc.x)) + abs(static_cast<int>(d.loc.y) -static_cast<int>(cvec[cid].loc.y));
    if(dist < min_dist){
      min_dist = dist;
    } 
  }
    /*std::cout<<"here\n";*/
  if (n_c_dist == min_dist && cvec[cid].requesting)
  {
    /*std::cout<<"here\n";*/
    /*cout_mutex.lock();*/
    /*std::cout << min_dist <<'\n';*/
    /*cout_mutex.unlock();*/
    cvec[cid].requesting = false;
    cvec[cid].did = did;
    dvec[did].cid = cid;
    dvec[did].free = false;
    match(cid, did);
    return true;
  } 
  return false;
}



void driver_accepting(unsigned int driver, location_t location){
  bool matched = false;
  while(!matched){//at ready state if not matched
    cvec_lock.lock();
    assert_interrupts_enabled();
    matched = try_match(driver);
    cvec_lock.unlock();
    if (matched) break;
    wait_driver++;
    cv_driver.wait(dvec_lock);
    assert_interrupts_enabled();
    wait_driver--;
     }
    cv_customer.broadcast();
    assert_interrupts_enabled();
}

void customer(uintptr_t arg){
  customer_arg* captr_ = reinterpret_cast<customer_arg*>(arg);
  std::unique_ptr<customer_arg> captr(captr_);
  for(int i = 0; i < static_cast<int>(captr->loc.size()); i++){
    assert(captr->loc.size() > 0);
    auto req_loc = captr->loc[i];
  thread::yield();
    assert_interrupts_enabled();
    cvec_lock.lock();
    assert_interrupts_enabled();
  thread::yield();
    assert_interrupts_enabled();
    cvec[captr->cid].loc = captr->loc[i];
    cvec[captr->cid].requesting = true;
    cvec[captr->cid].received = false;
    customer_ready(captr->cid, req_loc);// we don't want matching happen before us
    cv_driver.broadcast();
    assert_interrupts_enabled();
    while(cvec[captr->cid].requesting)//requesting
    {
      wait_customer++;
  thread::yield();
    assert_interrupts_enabled();
      cv_customer.wait(cvec_lock);
      wait_customer--;
    }
    while(!cvec[captr->cid].received){
      wait_customer_rc++;
      cv_customer.wait(cvec_lock); //TODO: this essentially different queue than the requesting customers
    assert_interrupts_enabled();
      wait_customer_rc--;
    }
    unsigned int did = cvec[captr->cid].did;
    cvec_lock.unlock();
  thread::yield();
    pay(captr->cid, did);
    assert_interrupts_enabled();
  thread::yield();
    dvec_lock.lock();
    assert_interrupts_enabled();
    dvec[did].paid = true;
    cv_driver.broadcast();
    assert_interrupts_enabled();
    dvec_lock.unlock();
    assert_interrupts_enabled();
    }
}



void driver(uintptr_t arg){
  driver_arg* daptr_ = reinterpret_cast<driver_arg*>(arg);
  std::unique_ptr<driver_arg> daptr(daptr_);
  while(1){
    dvec_lock.lock();
    location_t curr_pos = dvec[daptr->did].loc;
    driver_ready(daptr->did, curr_pos);
    dvec[daptr->did].free = true;
    driver_accepting(daptr->did, daptr->loc);
    cvec_lock.lock();
    location_t dest = cvec[dvec[daptr->did].cid].loc;
    location_t from = dvec[daptr->did].loc;
    cvec_lock.unlock();
  thread::yield();
    dvec_lock.unlock();
  thread::yield();
 
    //update the drivers' positions
    drive(daptr->did, from, dest);
  thread::yield();
    dvec_lock.lock();
    dvec[daptr->did].loc = dest;
    dvec[daptr->did].paid = false;
    unsigned int cid = dvec[daptr->did].cid;
    cvec_lock.lock();
    cvec[cid].received = true;
    cv_customer.broadcast();// TODO: might be not efficient
    cvec_lock.unlock();
  thread::yield();
    while(!dvec[daptr->did].paid){
      wait_driver++;
      cv_driver.wait(dvec_lock);// TODO: try waiting on a different queue instead of the waiting for request que
      wait_driver--;
    }
    dvec_lock.unlock();
  }
}

void init(uintptr_t arg){
  cvec_lock.lock();
  Ans* ans_ = reinterpret_cast<Ans*>(arg);
  std::unique_ptr<Ans> ans(ans_);
  char** files = ans->argv;
  //spawning drivers
  for(int i = 0; i < 10; i++){
  thread::yield();
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
    std::string input = "10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n10 20\n30 40\n50 60\n0 2\n3 4\n";  // Input data
    std::stringstream ss(input);
    customer_arg* cargptr = new(customer_arg);
    cargptr->cid = cnt;
    std::cout << "cid = " << cargptr->cid << '\n';
    std::vector<location_t> loc_vec;
    while(ss >> x >> y){
      loc_vec.push_back(location_t{x,y});
      /*std::cout<< x <<y << '\n';*/
    }
  thread::yield();
    cvec.push_back({(cargptr->loc = loc_vec)[0], false, false,UINT_MAX, static_cast<unsigned int>(cnt)});
    thread(customer, reinterpret_cast<uintptr_t>(cargptr));
  }
  cvec_lock.unlock();
}

int main(){
  Ans* ans = new(Ans);
    ans->argv = nullptr;
  ans->argc = 30;
  uintptr_t argv_ = reinterpret_cast<uintptr_t>(ans);
  cpu::boot(1, init, argv_,false, false, 0);
}

