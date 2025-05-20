#include <algorithm>
#include <cmath>
#include <deque>
#include <iostream>
#include <queue>
#include <random>
#include <set>
#include <unordered_map>
#include <vector>

#include "cpu.h"
#include "cv.h"
#include "mutex.h"
#include "thread.h"

// pizza functions
mutex cout_mutex;
struct location_t {
    unsigned int x;
    unsigned int y;
};
std::ostream& operator<<(std::ostream& os, const location_t& loc) {
    os << "(" << loc.x << ", " << loc.y << ")";
    return os;
}

void driver_ready(unsigned int driver, location_t location) {
    cout_mutex.lock();
    std::cout << "driver " << driver << "ready at " << location << '\n';
    cout_mutex.unlock();
}
void drive(unsigned int driver, location_t start, location_t end) {
    cout_mutex.lock();
    std::cout << "driver " << driver << "driving from " << start << " to " << end << '\n';
    cout_mutex.unlock();
}

void customer_ready(unsigned int customer, location_t location) {
    cout_mutex.lock();
    std::cout << "customer " << customer << "ready at " << location << '\n';
    cout_mutex.unlock();
}
void pay(unsigned int customer, unsigned int driver) {
    cout_mutex.lock();
    std::cout << "customer " << customer << " pays " << driver << '\n';
    cout_mutex.unlock();
}

void match(unsigned int customer, unsigned int driver) {
    cout_mutex.lock();
    std::cout << "customer " << customer << " matches with " << driver << '\n';
    cout_mutex.unlock();
}

// Global variables
uintptr_t total_customer = 20;
uintptr_t total_driver = 20;
std::deque<thread> threads;   // Deque to hold all of the initiated threads.customer locations

// Global variables for driver states and locationts and synchronization.
std::vector<int16_t> driver_state(total_driver);       // -2: paid | -1: ready | 0-C: matched
std::vector<mutex> mutex_driver_state(total_driver);   // Mutex for driver's state
std::vector<cv> cv_driver_state(total_driver);         // Condition variables for driver state changes
std::vector<location_t> driver_loc(total_driver);      // Driver locations
std::set<uintptr_t> ready_drivers;       // Set of ready drivers
mutex mutex_ready_driver;                // Mutex for ready_drivers set
cv cv_ready_driver;                      // Condition variable for ready_customers

// Global variables for customer states and locationts and synchronization.
std::vector<int16_t> customer_state(total_customer);                // -2: received | -1: ready | 0-D: matched
std::vector<mutex> mutex_customer_state(total_customer);            // Mutex for customer's state

std::set<uintptr_t> ready_customers;// Set of ready customers
std::vector<std::queue<location_t>>customer_loc(total_customer);

std::vector<cv> cv_customer_state(total_customer);         // Condition variables for customer state changes
mutex mutex_ready_customer;                         // Mutex for ready_customers set
cv cv_ready_customer;                               // Condition variable for ready_customers


/**
 * @brief Reads the orderring locations for a specific customer from a file.
 *
 * @param customer_num The ID of the customer.
 *
 * Reads the locations from the file specified in the global variable`customer_files[customer_num]`
 * and initiates the `customer_loc` queue for that customer.
 */
void read_customer_loc(uintptr_t customer_num) {
    std::mt19937 gen(42);                                 // Fixed seed for deterministic behavior
    std::uniform_int_distribution<int> dist_x(0, 1000);   // Adjust range as needed
    std::uniform_int_distribution<int> dist_y(0, 1000);

    location_t loc;
    for (int i = 0; i < 100; ++i) {   // Generate 100 random locations per customer
        loc.x = dist_x(gen);
        loc.y = dist_y(gen);
        customer_loc[customer_num].push(loc);
    }
  std::cout << "here\n";
}


/**
 * @brief Simulates a customer thread.
 *
 * @param customer_num The ID of the customer.
 *
 * The customer goes through the following lifecycle:
 * 1. Announces readiness at a location.
 * 2. Waits to be matched with a driver.
 * 3. Receives delivery from the driver.
 * 4. Pays the driver after delivery.
 * 5. Repeats for the next location until no more requests are available.
 */
void customer(uintptr_t customer_num) {
    read_customer_loc(customer_num);

    while (true) {
        // Check if customer has no more requests (empty queue) and end the thread.
        if (customer_loc[customer_num].empty()) {
            mutex_customer_state[customer_num].lock();
            customer_state[customer_num] = -3;
            mutex_customer_state[customer_num].unlock();
            return;
        }

        // Announce readiness at the current location (front of queue).
        mutex_ready_customer.lock();

        mutex_customer_state[customer_num].lock();
        customer_ready(customer_num, customer_loc[customer_num].front());
        customer_state[customer_num] = -1;
        mutex_customer_state[customer_num].unlock();

        ready_customers.insert(customer_num);
        cv_ready_customer.signal();
        mutex_ready_customer.unlock();

        // Wait to be matched.
        mutex_customer_state[customer_num].lock();
        while (customer_state[customer_num] == -1) {
            cv_customer_state[customer_num].wait(mutex_customer_state[customer_num]);
        }
        // Log the matched driver.
        uintptr_t matched_driver = customer_state[customer_num] - customer_num;
        customer_state[customer_num] = matched_driver;

        // Notify the matched driver.
        mutex_driver_state[matched_driver].lock();
        driver_state[matched_driver] = customer_num;
        cv_driver_state[matched_driver].signal();
        mutex_driver_state[matched_driver].unlock();

        mutex_customer_state[customer_num].unlock();

        // Wait for delivery.
        mutex_customer_state[customer_num].lock();
        while (customer_state[customer_num] != -2) {
            cv_customer_state[customer_num].wait(mutex_customer_state[customer_num]);
        }
        mutex_customer_state[customer_num].unlock();

        // Pay the driver.
        pay(customer_num, matched_driver);
        // Mark the driver as paid
        mutex_driver_state[matched_driver].lock();
        driver_state[matched_driver] = -2;
        cv_driver_state[matched_driver].signal();
        mutex_driver_state[matched_driver].unlock();

        // Move to the next location.
        customer_loc[customer_num].pop();
    }
}


/**
 * @brief Simulates a driver thread.
 *
 * @param customer_num The ID of the driver.
 *
 * The driver goes through the following lifecycle:
 * 1. Announces readiness at a location.
 * 2. Waits to be matched with a customer.
 * 3. Delivers pizza to the matched customer.
 * 4. Gets paid by the matched customer.
 * 5. Repeats for the next location until no more customer requests are coming.
 */
void driver(uintptr_t driver_num) {
    // Initialize the driver's location
    driver_loc[driver_num] = { 0, 0 };

    while (true) {
        // Announce readiness at the current location.
        mutex_ready_driver.lock();

        mutex_driver_state[driver_num].lock();
        driver_ready(driver_num, driver_loc[driver_num]);
        driver_state[driver_num] = -1;
        mutex_driver_state[driver_num].unlock();

        ready_drivers.insert(driver_num);
        cv_ready_driver.signal();
        mutex_ready_driver.unlock();

        // Wait to be matched.
        mutex_driver_state[driver_num].lock();
        while (driver_state[driver_num] == -1) {
            cv_driver_state[driver_num].wait(mutex_driver_state[driver_num]);
        }
        // Log the matched customer.
        uintptr_t matched_customer = driver_state[driver_num];
        mutex_driver_state[driver_num].unlock();

        // Drive to the matched customer.
        location_t destination = customer_loc[matched_customer].front();
        drive(driver_num, driver_loc[driver_num], destination);
        driver_loc[driver_num] = destination;

        // Mark the matched customer as received.
        mutex_customer_state[matched_customer].lock();
        customer_state[matched_customer] = -2;
        cv_customer_state[matched_customer].signal();
        mutex_customer_state[matched_customer].unlock();

        // Wait to be paid.
        mutex_driver_state[driver_num].lock();
        while (driver_state[driver_num] != -2) {
            cv_driver_state[driver_num].wait(mutex_driver_state[driver_num]);
        }
        mutex_driver_state[driver_num].unlock();
    }
}


/**
 * @brief Calculates the Manhattan distance between two locations
 *
 * @param a The first location as a 'location_t' structure.
 * @param b The second location as a 'location_t' structure.
 *
 * @return The Manhattan distance (integer) between the two locations.
 */
int get_distance(location_t a, location_t b) {
    return std::abs(static_cast<int>(a.x - b.x)) + std::abs(static_cast<int>(a.y - b.y));
}

/**
 * @brief Find the closest driver to the target customer.
 *
 * @param customer_num The ID of the target customer.
 *
 * @return The ID of the closest driver.
 *
 * @note Ties are broken by choosing the driver with a smaller ID.
 */
uintptr_t find_closest_driver(size_t customer_num) {
    int distance = -1;
    uintptr_t target_driver;

    for (auto& driver : ready_drivers) {
        int tmp = get_distance(driver_loc[driver], customer_loc[customer_num].front());
        if (distance == -1 || tmp < distance) {
            distance = tmp;
            target_driver = driver;
        }
    }

    return target_driver;
}

/**
 * @brief Find the closest customer to the target driver.
 *
 * @param driver_num The ID of the target driver.
 *
 * @return The ID of the closest customer.
 *
 * @note Ties are broken by choosing the customer with a smaller ID.
 */
uintptr_t find_closest_customer(size_t driver_num) {
    int distance = -1;
    uintptr_t target_customer;

    for (auto& customer : ready_customers) {
        int tmp = get_distance(driver_loc[driver_num], customer_loc[customer].front());
        if (distance == -1 || tmp < distance) {
            distance = tmp;
            target_customer = customer;
        }
    }

    return target_customer;
}

/**
 * @brief Simuates the matching thread.
 *
 * @param arg Unused variable for running the thread.
 *
 * The matching thread goes through the following lifecycle:
 * 1. Waits for at least one customer to announce readiness.
 * 2. Stops accepting new ready customers.
 * 3. Waits for at least one driver to announce readiness.
 * 4. Stops accepting new ready drivers.
 * 5. Make all of the possible matches.
 *      - Matching Process:
 *          - Set the ready customer with the smallest ID as the current customer
 *          - Finds the closest driver to the current customer, set as the current driver
 *          - Finds the closest customer to the current driver
 *          - If the current customer is the closest customer, a match is established
 *          - Otherwise, set the closest customer as the current customer and repeat
 * 6. Allow new ready customers/drivers.
 */
void make_match(uintptr_t arg) {
    while (true) {
        // Wait until at least one customer is ready to enter match.
        // Forbid any other customers from announcing ready once in matching.
        mutex_ready_customer.lock();
        while (ready_customers.size() == 0) {
            cv_ready_customer.wait(mutex_ready_customer);
        }

        // Wait until at least one driver is ready to enter match.
        // Forbid any other drivers from announcing ready once in matching.
        mutex_ready_driver.lock();
        while (ready_drivers.size() == 0) {
            cv_ready_driver.wait(mutex_ready_driver);
        }

        // Continue until all possible matches are made
        while (ready_customers.size() > 0 && ready_drivers.size() > 0) {
            uintptr_t current_customer = *ready_customers.begin();
            uintptr_t closest_driver = find_closest_driver(current_customer);
            uintptr_t closest_customer = find_closest_customer(closest_driver);

            // Find a dual match
            while (current_customer != closest_customer) {
                current_customer = closest_customer;
                closest_driver = find_closest_driver(current_customer);
                closest_customer = find_closest_customer(closest_driver);
            }

            // Perform match
            match(closest_customer, closest_driver);

            // Notify customer
            mutex_customer_state[closest_customer].lock();
            customer_state[closest_customer] = closest_driver + closest_customer;
            cv_customer_state[closest_customer].signal();
            mutex_customer_state[closest_customer].unlock();

            // Remove matched pairs
            ready_drivers.erase(closest_driver);
            ready_customers.erase(closest_customer);
        }

        // Allow more customers/drivers to announce ready
        mutex_ready_customer.unlock();
        mutex_ready_driver.unlock();
    }
}


/**
 * @brief Simulates a multi-thread pizza delivery system
 *
 * @param total_driver_num The total number of drivers available in the system.
 *
 * The system creates one separate thread for each driver and each customer.
 * Another separate thread is created for establishing match between customers and drivers.
 */
void pizza_delivery_system(uintptr_t total_driver_num) {
    // Initiate drivers in the system
    driver_state.resize(total_driver_num, -4);
    // mutex_driver_state.resize(total_driver_num);
    // cv_driver_state.resize(total_driver_num);
    driver_loc.resize(total_driver_num);

    // Initiate customers in the system
    uintptr_t total_customer_num = total_customer;
    customer_state.resize(total_customer_num, -4);
    // mutex_customer_state.resize(total_customer_num);
    // cv_customer_state.resize(total_customer_num);
    customer_loc.resize(total_customer_num);

    // Create threads for drivers
    for (uintptr_t driver_num = 0; driver_num < total_driver_num; ++driver_num) {
        threads.emplace_back(driver, driver_num);
        // break;
    }

    // Create threads for customers
    for (uintptr_t customer_num = 0; customer_num < total_customer_num; ++customer_num) {
        threads.emplace_back(customer, customer_num);
        // break;
    }

    // Create a thread for making match
    threads.emplace_back(make_match, total_customer_num);
}

/**
 * @brief Main function of the pizza delivery system.
 *
 * @param argc The total number of arguments passed to the program.
 * @param argv Array of character pointers containing the command-line arguments.
 *
 * @return int Returns `0` upon successful execution.
 */
int main() {
    // initialize customer location
    for (uintptr_t i = 0; i < total_customer; i++) {
        read_customer_loc(i);
    }
    // Create the initial thread.
    cpu::boot(1, pizza_delivery_system, total_driver, false, false, 0);
    // Make sure all created threads have exited.
    for (auto& t : threads) {
        t.join();
    }
    return 0;
}
