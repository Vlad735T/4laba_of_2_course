#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include <random>
#include <iomanip> 

using namespace std;

struct TIME{
    int seconds, minutes, hours;

    bool valid() const {
        return hours >= 0 && hours <= 23 &&
        minutes >= 0 && minutes <= 59 &&
        seconds >= 0 && seconds <= 59;
    }
};

ostream& operator<<(ostream& os, const TIME& time ){
    os << time.hours << " : " 
    << time.minutes << " : " 
    << time.seconds;
    return os;
}

vector<vector<TIME>> generate_times(size_t num_of_time, size_t size_of_time) {

    vector<vector<TIME>> times;

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> hours_dist(0, 46);    
    uniform_int_distribution<> mins_secs_dist(0, 117); 

    // Вычисляем количество групп 
    size_t group_count = (num_of_time + size_of_time - 1) / size_of_time;
    times.resize(group_count);

    for (auto& group : times) {
        // Определяем размер текущей группы
        size_t current_group_size = min(num_of_time, size_of_time);
        group.resize(current_group_size);
        num_of_time -= current_group_size;

        for (auto& time : group) {
            time.hours = hours_dist(gen);
            time.minutes = mins_secs_dist(gen);
            time.seconds = mins_secs_dist(gen);
        }
    }

    return times;
}



vector<TIME> single_thread_check(const vector<vector<TIME>>& times) {
    vector<TIME> error_times;

    for (const auto& group : times) {
        for (const auto& time : group) {
            if (!time.valid()) {
                error_times.push_back(time);
            }
        }
    }
    return error_times;
}

vector<TIME> multi_thread_check(const vector<vector<TIME>>& times, const int& cout_of_thread) {

    vector<TIME> error_times;
    mutex error_mutex; // for pravilnogo add in vector

    auto worker = [&](int start, int end){ // for work thread

        vector<TIME> local_errors;
        for(int i = start; i < end; i++){

            for(const auto& time: times[i]){

                if (!time.valid()){
                    local_errors.push_back(time);
                }
            }
        }
        lock_guard<mutex> lock(error_mutex);
        error_times.insert(error_times.end(), local_errors.begin(), local_errors.end());
    };



    vector<thread> thread;
    int cout_of_process_data  = times.size() / cout_of_thread;
    int other_cout_of_process_data  = times.size() % cout_of_thread;

    size_t start = 0;
    for(size_t i = 0; i < cout_of_thread; i++){
        size_t end = start + cout_of_process_data + (i < other_cout_of_process_data ? 1 : 0);

        thread.emplace_back(worker, start, end);
        start = end;
    }

    for(auto& t : thread){
        t.join();
    }

    return error_times;
}




int main(){

    int num_of_time = 100;
    int size_of_time = 3;
    int cout_of_thread = 4;

    auto group_times = generate_times(num_of_time, size_of_time);

    // Однопоточная проверка
    auto start_single = chrono::high_resolution_clock::now();
    auto single_errors = single_thread_check(group_times);
    auto end_single = chrono::high_resolution_clock::now();
    chrono::duration<double> single_duration = end_single - start_single; // time difference

    // Многопоточная проверка   
    auto start_multi = chrono::high_resolution_clock::now();
    auto multi_errors = multi_thread_check(group_times, cout_of_thread);
    auto end_multi = chrono::high_resolution_clock::now();
    chrono::duration<double> multi_duration = end_multi - start_multi; 

    cout << fixed << setprecision(4);

    cout << "Однопоточная обработка заняла: " << single_duration.count() * 1000 << " миллисекунд (" 
        << single_duration.count() << " секунд)\n";
    cout << "Многопоточная обработка заняла: " << multi_duration.count() * 1000 << " миллисекунд ("
        << multi_duration.count() << " секунд)\n";

    cout << "Некорректных данных (однопоточно): " << single_errors.size() << "\n";
    cout << "Некорректных данных (многопоточно): " << multi_errors.size() << "\n";

    cout << "\nНекорректные данные (пример):\n";
    for (size_t i = 0; i < single_errors.size(); i++) {
        cout << single_errors[i] << "\n";
    }

    return 0;
}