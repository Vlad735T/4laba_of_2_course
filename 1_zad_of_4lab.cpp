#include <iostream>
#include <thread>
#include <mutex>
#include <semaphore>
#include <barrier>
#include <atomic>
#include <condition_variable>
#include <chrono>
#include <vector>
#include <random>

using namespace std;

// Примитивы синхронизации
mutex mtx;
counting_semaphore<3> semaphore(3); // Максимум 3 потока могут работать одновременно
atomic_flag spinLock = ATOMIC_FLAG_INIT; // блокировка для Спинлока (изначально False- доступен)

// Функция для генерации случайного символа из ASCII  
char generate_random_char() {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(32, 126); // символы с кодами(которые доступны на клавиатуре)
    return static_cast<char>(dis(gen));
}


//  Monitor
class Monitor {
public:
    Monitor() : flag(false) {} // конструктор,который создает доступный монитор

    // Метод для захвата монитора
    void enter() {
        unique_lock<mutex> lock(mtx); //пытаемся захватить монитор
        while (flag) {  // проверка доступен ли монитор
            cond.wait(lock);  // если монитор занят,то нахоидся в сотоянии ожидания
        }
        flag = true; // захватываем монитор
    }

    // Метод для освобождения монитора
    void exit() {
        lock_guard<mutex> lock(mtx); // захватываем мьютексом чтобы поменять флаг
        flag = false; // меняем флаг ( монитор свободен)
        cond.notify_one();  // уведомляем другой поток,что монитор свободен
    }

private:
    bool flag;  // Флаг для контроля доступа
    mutex mtx;
    condition_variable cond; //переменная для уведомления потоков
};

// Тестирование Monitor
void test_monitor(int num_threads) {
    cout << "Тестирование Monitor:" << endl;

    Monitor monitor;  // создаем монитор
    vector<thread> threads; //вектор для потоков

    auto start = chrono::high_resolution_clock::now(); // таймер начала

    // Создаем потоки, которые будут работать с Monitor
    for (int i = 0; i < num_threads; ++i) {
        threads.push_back(thread([i, &monitor]() {
            monitor.enter();  // захватываем монитор
            {
              //  lock_guard<mutex> lock(mtx); 
                cout << "Поток " << i << " работает с общим ресурсом с использованием Monitor." << endl;
            }
            monitor.exit();  // освобождаем монитор
            }));
    }

    // Ожидаем завершения всех потоков
    for (auto& t : threads) {
        t.join();
    }

    auto end = chrono::high_resolution_clock::now(); // таймер конца
    chrono::duration<double> duration = end - start; // время выполнения
    cout << "Время работы Monitor: " << duration.count() << " секунд." << endl;
    cout << "---------------------------------" << endl;
}


//  SemaphoreSlim
class SemaphoreSlim {
public:
    SemaphoreSlim(int count) : count(count) {}

    // Метод для захвата семафора
    void wait() {
        unique_lock<mutex> lock(mtx); //пытаемся захватить ресурс
        while (count == 0) { // проверка на свободные ресурсы
            cond.wait(lock);  // Ожидаем освобождения ресурса
        }
        --count;  // Уменьшаем счетчик(захватываем ресурс)
    }

    // Метод для освобождения семафора
    void release() {
        lock_guard<mutex> lock(mtx); // захыватыаем поток,Чтобы изменить счетчик
        ++count;  // Увеличиваем счетчик(освобождая ресурс)
        cond.notify_one();  // Уведомляем один поток,что ресурс свободен
    }

private:
    int count;// сколько потоков могут использовать ресурс
    mutex mtx;
    condition_variable cond;
};


// Тестирование SemaphoreSlim
void test_semaphore_slim(int num_threads) {
    cout << "Тестирование SemaphoreSlim:" << endl;

    SemaphoreSlim sem(3);  // Семафор с максимумом 3 потоков
    vector<thread> threads; //вектор для потоков

    auto start = chrono::high_resolution_clock::now(); // таймер начала

    // Создаем потоки, которые будут работать с SemaphoreSlim
    for (int i = 0; i < num_threads; ++i) {
        threads.push_back(thread([i, &sem]() {
            sem.wait();  // Поток ожидает, пока не освободится ресурс
            {
                lock_guard<mutex> lock(mtx); // мьютекс для вывода
                cout << "Поток " << i << " работает с общим ресурсом с использованием SemaphoreSlim." << endl;
            }
            sem.release();  // Освобождаем ресурс для других потоков
            }));
    }

    // Ожидаем завершения всех потоков
    for (auto& t : threads) {
        t.join();
    }

    auto end = chrono::high_resolution_clock::now(); // таймер конца
    chrono::duration<double> duration = end - start; // длительность
    cout << "Время работы SemaphoreSlim: " << duration.count() << " секунд." << endl;
    cout << "---------------------------------" << endl;
}

// Тестирование мьютекса
void test_mutex(int num_threads) {
    cout << "Тестирование мьютекса:" << endl;

    vector<thread> threads; //вектор для потоков

    auto start = chrono::high_resolution_clock::now(); // таймер начала

    // Создаем потоки, которые будут работать с мьютексом
    for (int i = 0; i < num_threads; ++i) {
        threads.push_back(thread([i]() { // создаем поток и добавляем в вектор
            lock_guard<mutex> lock(mtx); // захватываем мьютексом вывод
            cout << "Поток " << i << " работает с общим ресурсом с использованием мьютекса." << endl;
            }));
    }

    // Ожидаем завершения всех потоков
    for (auto& t : threads) {
        t.join();
    }

    auto end = chrono::high_resolution_clock::now(); // таймер конца
    chrono::duration<double> duration = end - start; // вычисляем разницу таймером

    cout << "Время работы мьютекса: " << duration.count() << " секунд." << endl;
    cout << "---------------------------------" << endl;
}

// Тестирование семафора(библиотека)
void test_semaphore(int num_threads) {
    cout << "Тестирование Semaphore:" << endl;

    vector<thread> threads;

    auto start = chrono::high_resolution_clock::now(); // Начало замера времени

    // Создаем семафор с ограничением на 3 потока
    counting_semaphore<2> sem(2);

    // Создаем потоки, которые будут работать с семафором
    for (int i = 0; i < num_threads; ++i) {
        threads.push_back(thread([i, &sem]() {
            sem.acquire();  // Поток ожидает, пока не освободится место
            {
                lock_guard<mutex> lock(mtx);
                cout << "Поток " << i << " работает с общим ресурсом с использованием семафора." << endl;
            }
            sem.release();  // Освобождаем место для других потоков
            }));
    }

    // Ожидаем завершения всех потоков
    for (auto& t : threads) {
        t.join();
    }

    auto end = chrono::high_resolution_clock::now(); // Конец замера времени
    chrono::duration<double> duration = end - start;
    cout << "Время работы семафора: " << duration.count() << " секунд." << endl;
    cout << "---------------------------------" << endl;
}

// Тестирование SpinLock
void test_spinlock(int num_threads) {
    cout << "Тестирование SpinLock:" << endl;

    vector<thread> threads;

    auto start = chrono::high_resolution_clock::now(); // таймер начало

    // Создаем потоки, которые будут работать с SpinLock
    for (int i = 0; i < num_threads; ++i) {
        threads.push_back(thread([i]() { // создаем поток и добавляем в вектор
            while (spinLock.test_and_set()) { // проверка флага блокировки и если он доступен,то устанавливаем его в True(захватываем флаг) ; постоянная проверка флага
            }
            {
                //lock_guard<mutex> lock(mtx);
                cout << "Поток " << i << " работает с общим ресурсом с использованием SpinLock." << endl;
            }
            spinLock.clear(); // Освобождаем SpinLock , сбрасывая флаг на False
            }));
    }

    // Ожидаем завершения всех потоков
    for (auto& t : threads) {
        t.join();
    }

    auto end = chrono::high_resolution_clock::now(); // таймер конец
    chrono::duration<double> duration = end - start; // длительность

    cout << "Время работы SpinLock: " << duration.count() << " секунд." << endl;
    cout << "---------------------------------" << endl;
}

// Тестирование Barrier
void test_barrier(int num_threads) {
    cout << "Тестирование Barrier:" << endl;

    vector<thread> threads;
    barrier bar(num_threads); // Блокировка, пока все потоки не достигнут барьера

    auto start = chrono::high_resolution_clock::now(); // таймер начало

    // Создаем потоки, которые будут работать с Barrier
    for (int i = 0; i < num_threads; ++i) {
        threads.push_back(thread([i, &bar]() { // создаем поток и добавляем в вектор
            {
                lock_guard<mutex> lock(mtx); // мьютекс для вывода (ресурсы до барьера)
                cout << "Поток " << i << " достиг барьера." << endl;
            }
            bar.arrive_and_wait(); // поток дошел до барьера и ждет (как только все 3 потока дойдут до сюда,следующий ресурс откроется)
            {
                lock_guard<mutex> lock(mtx); // мьютекс для вывода( ресурсы после барьера)
                cout << "Поток " << i << " прошел барьер." << endl;
            }
            }));
    }

    // Ожидаем завершения всех потоков
    for (auto& t : threads) {
        t.join();
    }

    auto end = chrono::high_resolution_clock::now(); // таймер конец
    chrono::duration<double> duration = end - start; //длительность
    cout << "Время работы Barrier: " << duration.count() << " секунд." << endl;
    cout << "---------------------------------" << endl;
}

// Тестирование SpinWait
void test_spinwait(int num_threads) {
    cout << "Тестирование SpinWait:" << endl;

    vector<thread> threads;

    auto start = chrono::high_resolution_clock::now(); // таймер начало

    // Создаем потоки, которые будут работать с SpinWait
    for (int i = 0; i < num_threads; ++i) {
        threads.push_back(thread([i]() { // создаем поток и добавляем в вектор
            while (spinLock.test_and_set()) { // проверка флага блокировки и если он доступен,то устанавливаем его в True(захватываем флаг) ; постоянная проверка флага
                this_thread::yield();
            }
            {
                //lock_guard<mutex> lock(mtx);
                cout << "Поток " << i << " работает с общим ресурсом с использованием SpinWait." << endl;
            }
            spinLock.clear(); // Освобождаем SpinLock , сбрасывая флаг на False
            }));
    }

    // Ожидаем завершения всех потоков
    for (auto& t : threads) {
        t.join();
    }

    auto end = chrono::high_resolution_clock::now(); // таймер конец
    chrono::duration<double> duration = end - start; // длительность

    cout << "Время работы SpinLock: " << duration.count() << " секунд." << endl;
    cout << "---------------------------------" << endl;
}


int main() {
    setlocale(LC_ALL, "ru");
    cout << "Запуск потоков, генерирующих случайные символы:" << endl;

    const int num_threads = 3; // количество потоков
    vector<thread> threads; // вектор для потоков

    // Создаем потоки для генерации случайных символов
    for (int i = 0; i < num_threads; ++i) {
        threads.push_back(thread([i]() { // создаем поток и добавляем в вектор
            for (int j = 0; j < 3; ++j) {
                char c = generate_random_char(); // генерируем случайный символ
                {
                    lock_guard<mutex> lock(mtx);// захватываем мьютексом вывод
                    cout << "Поток " << i << ": " << c << endl;
                }
                this_thread::sleep_for(chrono::milliseconds(3000));// задержка
            }
            }));
    }

    // Ожидаем завершения всех потоков
    for (auto& t : threads) {
        t.join();
    }

    cout << "Завершение работы всех потоков." << endl;
    cout << "---------------------------------" << endl;

    // Тестируем примитивы
    test_mutex(num_threads);
    test_semaphore(num_threads);
    test_spinlock(num_threads);
    test_barrier(num_threads);
    test_spinwait(num_threads);
    test_monitor(num_threads);
    test_semaphore_slim(num_threads);

    return 0;
}