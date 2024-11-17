#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>   
#include <random>
#include <iomanip>
#include <condition_variable>

using namespace std;

const int num_of_philosophers = 5;

class Waiter{
private:
    mutex mtx;
    condition_variable philosophers_waiter;
    vector<bool> forks;
public:

    Waiter(): forks(num_of_philosophers, true) {}

    void request_forks(int id_philosopher){
        unique_lock<mutex> lock(mtx);
        while(!forks[id_philosopher] || !forks[(id_philosopher + 1) % num_of_philosophers]){
            philosophers_waiter.wait(lock);
        }
        forks[id_philosopher] = false;
        forks[(id_philosopher + 1) % num_of_philosophers] = false;
    }

    void return_forks(int id_philosopher){
        lock_guard<mutex> lock(mtx);
        forks[id_philosopher] = true;
        forks[(id_philosopher + 1) % num_of_philosophers] = true;
        philosophers_waiter.notify_all();
    }
};

// Мьютекс для синхронизации вывода
mutex cout_mutex;

void philosopher(int id, Waiter& waiter){

    while(true){

        {
            lock_guard<mutex> lock(cout_mutex);
            cout << "Философ " << id << " размышляет" << "\n";
        }
        this_thread::sleep_for(chrono::milliseconds(1000));

        waiter.request_forks(id);

        {
            lock_guard<mutex> lock(cout_mutex);
            cout << "Философ " << id << " кушает" << "\n";
        }
        this_thread::sleep_for(chrono::milliseconds(1500));

        {
            lock_guard<mutex> lock(cout_mutex);
            cout << "Философ " << id << " освободил вилки" << "\n";
        }

        waiter.return_forks(id);
    }
}

int main(){

    Waiter waiter;
    vector<thread>  philosophers;

    for(int i = 0; i < num_of_philosophers; i++){
        philosophers.emplace_back(philosopher, i, ref(waiter));
    }

    for(auto& t : philosophers){
        t.join();
    }

    return 0;
}