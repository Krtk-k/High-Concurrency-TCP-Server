#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <functional>

using namespace std;

class ThreadPool {
    bool end;
    queue<function<void()>> tasks;
    int n_threads = 3;
    vector<thread> workers;
    condition_variable cv;
    mutex mtx;
    void worker() {
        while(1) {
            function<void()> curr;
            {
                unique_lock<mutex> lock(mtx);
                cv.wait(lock, [this]{return !tasks.empty() || end;});
                if(end && tasks.empty()) return;
                curr = tasks.front();
                tasks.pop();
            }
            curr();
        }
    }
    public:
    ThreadPool() {
        end = false;
        for(int i = 0; i<n_threads; i++) {
            workers.push_back(thread([this]{worker();}));
        }
    }
    void add_task(function<void()> func) {
        lock_guard<mutex> lock(mtx);
        tasks.push(func);
        cv.notify_one();
    }
    ~ThreadPool() {
        end = true;
        cv.notify_all();

        for(auto &t : workers) {
            if(t.joinable()) t.join();
        }
    }
};