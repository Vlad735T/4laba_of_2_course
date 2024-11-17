#include <iostream>
#include <string>
#include <cctype>
#include <thread>
#include <vector>
#include <random>
#include <chrono>
#include <mutex>
#include <semaphore>
#include <barrier>
#include <queue>
#include <condition_variable>
#include <atomic>
using namespace std;

mutex mtx;

char gen_rand_symbol(){
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> symbol(33, 126);
    
    return static_cast<char>(symbol(gen));
}


void race_test(int count_of_threads) {
    vector<thread> threads;

    auto start = chrono::high_resolution_clock::now();

    for (int i = 0; i < count_of_threads; i++) {
        threads.emplace_back([i]() { 
            for (int j = 0; j < 2; j++) {
                char sym = gen_rand_symbol();{
                    lock_guard<mutex> lock(mtx);
                    cout << "Number of thread: " << i << " generation a symbol: " << sym << endl;
                }
                this_thread::sleep_for(chrono::milliseconds(30));
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> duration = end - start;
    cout << "Execution time: " << duration.count() << " seconds" << endl;
}

void mutexes(int count_of_threads) { 
    cout << "------------------------------------------------" << "\n";
    cout << "Checking the operation of MUTEXES" << "\n";
    auto start = chrono::high_resolution_clock::now();

    vector<thread> threads; 
    for (int i = 0; i < count_of_threads; i++) {
        threads.emplace_back([i]() {
            mtx.lock(); 
            cout << "Mutex locked by thread " << i 
            << "! Shared resource used by thread " << i;
            mtx.unlock(); 
            cout << "! Mutex unlocked by thread " << i << "\n";
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    auto end = chrono::high_resolution_clock::now(); 
    chrono::duration<double> duration = end - start;
    
    lock_guard<mutex> lock(mtx); // Защита вывода времени
    cout << "Execution time: " << duration.count() << " seconds" << endl;
    cout << "------------------------------------------------" << "\n";
}


void semaphore_test(int numberOfThreads) {  
    cout << "Semaphore test" << endl;
    auto start = chrono::high_resolution_clock::now(); 

    vector<thread> threads;  
    counting_semaphore<3> semaphore(3); // Пропускает 3 потока одновременно

    for (int i = 0; i < numberOfThreads; i++) {
        threads.push_back(thread([i, &semaphore]() {
            semaphore.acquire(); // try get semaphore
            {
                lock_guard<mutex> lock(mtx);
                cout << "Acquired semaphore by thread " << i 
                << "! Using shared resources by thread " << i;
                cout << "! Released semaphore by thread " << i << "\n";
            }
            semaphore.release();
        }));
    }

    for (auto& t : threads) {
        t.join();  
    }

    auto end = chrono::high_resolution_clock::now(); 
    chrono::duration<double> duration = end - start; 
    this_thread::sleep_for(chrono::milliseconds(200));

    lock_guard<mutex> lock(mtx); // Защита вывода времени
    cout << "Execution time: " << duration.count() << " seconds" << endl;
    cout << "------------------------------------------------" << "\n";
}

void semaphore_slim_test(int cout_of_threads){
    cout << "SemaphoreSlim test" << "\n";
    auto start = chrono::high_resolution_clock::now();

    vector<thread> threads;  
    counting_semaphore<1> semaphore(1); 

    for (int i = 0; i < cout_of_threads; i++){
        threads.push_back(thread([i, &semaphore](){
            semaphore.acquire(); // Поток пытается захватить семафор
            {
                lock_guard<mutex> lock(mtx); 
                cout << "Thread " << i << " acquired semaphore! " << "Using shared resources! ";
                cout << "Thread " << i << " released semaphore" << endl;
   
            }
            semaphore.release(); // Поток освобождает семафор
        }));
    }

    for (auto& t : threads) {
        t.join();  
    }

    auto end = chrono::high_resolution_clock::now(); 
    chrono::duration<double> duration = end - start; 
    this_thread::sleep_for(chrono::milliseconds(200));

    lock_guard<mutex> lock(mtx); 
    cout << "Execution time: " << duration.count() << " seconds" << endl;
    cout << "------------------------------------------------" << "\n";
}


void barrier_test(int cout_of_threads){

    cout << "BARRIER test" << "\n";
    auto start = chrono::high_resolution_clock::now(); 

    vector<thread> threads;  
    barrier barrierLock(cout_of_threads);  // Барьер, ожидающий, пока все потоки не достигнут этой точки

    for (int i = 0; i < cout_of_threads; i++){
        threads.push_back(thread([i, &barrierLock](){

            {
                lock_guard<mutex> lock(mtx);
                cout << "Thread " << i << " reached the barrier." << endl;            
            }

            barrierLock.arrive_and_wait(); // Поток достигает барьера и ждет других

            {
                lock_guard<mutex> lock(mtx);
                cout << "Thread " << i << " passed the barrier." << endl;  
            }
        }));
    }

    for (auto& t : threads) {
        t.join();
    }

    auto end = chrono::high_resolution_clock::now(); 
    chrono::duration<double> duration = end - start; 

    lock_guard<mutex> lock(mtx); 
    cout << "Execution time: " << duration.count() << " seconds" << endl;
    cout << "------------------------------------------------" << "\n";
}


class Spinlock{
private:
//atomary flags  (t, f)
    std::atomic_flag atomic_flag = ATOMIC_FLAG_INIT; // flag ne ystanovlen!!

public:
    void lock(){
         // Ожидаем, пока atomic_flag не станет "свободным" (не установленным)
        while (atomic_flag.test_and_set(std::memory_order_acquire)){}
    }

// Освобождаем лок
    void unlock(){
        atomic_flag.clear(std::memory_order_release);
    }
};

void spinLock_test(int cout_of_threads) {
    cout << "SPINLOCK test" << "\n";
    auto start = chrono::high_resolution_clock::now();
    Spinlock slock;  // Создаем объект spinlock
    vector<thread> threads;  

    for (int i = 0; i < cout_of_threads; i++) {
        threads.push_back(thread([i, &slock]() {
            slock.lock();
            {
                lock_guard<mutex> lock(mtx);
                cout << "Thread " << i << " acquired lock! " << "Using shared resources! ";
                cout << "Thread " << i << " released lock" << endl;
            }
            slock.unlock();
        }));
    }

    for (auto& t : threads) {
        t.join();
    }

    auto end = chrono::high_resolution_clock::now(); 
    chrono::duration<double> duration = end - start;  

    lock_guard<mutex> lock(mtx);  
    cout << "Execution time: " << duration.count() << " seconds" << endl;
    cout << "------------------------------------------------" << "\n";
}


class Spinwait {
private:
    atomic<bool> flag = false;

public:
    void lock() {
        while (flag.exchange(true, memory_order_acquire)) { // Попытка захватить лок, если он уже захвачен, поток передает управление другому потоку
            std::this_thread::yield();
        }
    }

    void unlock() {
        flag.store(false, memory_order_release);
    }
};

void spinWait_test(int cout_of_threads) {
    cout << "SPINWAIT test" << "\n";
    auto start = chrono::high_resolution_clock::now();  
    Spinwait swlock;  
    vector<thread> threads; 

    for (int i = 0; i < cout_of_threads; i++) {
        threads.push_back(thread([i, &swlock]() {
            swlock.lock();
            {
                lock_guard<mutex> lock(mtx);
                cout << "Thread " << i << " acquired lock! " << "Using shared resources! ";
                cout << "Thread " << i << " released lock" << endl;
            }
            swlock.unlock();
        }));
    }

    for (auto& t : threads) {
        t.join();
    }

    auto end = chrono::high_resolution_clock::now(); 
    chrono::duration<double> duration = end - start;  

    lock_guard<mutex> lock(mtx);  
    cout << "Execution time: " << duration.count() << " seconds" << endl;
    cout << "------------------------------------------------" << "\n";
}


class Monitor {
private:
    mutex mtx;
    condition_variable cv;
    queue<char> dataQueue;

public:
    void addData(char value) {
        unique_lock<mutex> lock(mtx);
        dataQueue.push(value);
        cout << "Data added: " << value << endl;
        cv.notify_one(); // уведомление для продолжения работы другого потока
    }
};

void monitor_test(int cout_of_threads) {
    cout << "MONITOR test" << "\n";
    auto start = chrono::high_resolution_clock::now();
    Monitor monitor;
    vector<thread> threads;

    for (int i = 0; i < cout_of_threads; i++) {
        threads.push_back(thread([i, &monitor]() {
            monitor.addData(gen_rand_symbol()); 
        }));
    }

    for (auto& t : threads) {
        t.join();
    }

    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> duration = end - start;

    lock_guard<mutex> lock(mtx);
    cout << "Execution time: " << duration.count() << " seconds" << endl;
    cout << "------------------------------------------------" << "\n";
}


int main(){

    int cout_of_threads = 0;
    while (true) {
        cout << "Enter the number of running threads: ";
        
        string input;
        cin >> input;

        bool isValid = true;
        for (char c : input) {
            if (!isdigit(c)) {
                isValid = false;
                break;
            }
        }

        if (!isValid) {
            cout << "Некорректный ввод! Введите целое положительное число." << endl;
            continue;
        } 

        cout_of_threads = stoi(input);

        if (cout_of_threads <= 0) {
            cout << "The number of threads must be positive!" << endl;
            continue;
        }
        break;
    }

    cout << "------------------------------------------------" << "\n";
    race_test(cout_of_threads);
    mutexes(cout_of_threads);
    semaphore_test(cout_of_threads);
    semaphore_slim_test(cout_of_threads);
    barrier_test(cout_of_threads);
    spinLock_test(cout_of_threads);
    spinWait_test(cout_of_threads);
    monitor_test(cout_of_threads);

    return 0;
}
