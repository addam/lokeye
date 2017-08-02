#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <vector>
#include <memory>

using std::vector;

template<typename T>
class Safe : public T {
    std::atomic_flag locked = ATOMIC_FLAG_INIT;
public:
    void lock() {
        while (locked.test_and_set()) {
            std::this_thread::yield();
        }
    }
    void unlock() {
        locked.clear();
    }
};

using Measurements = Safe<vector<int>>;
using Lock = std::lock_guard<Measurements>;
using Gaze = int;

void reader_thread(Measurements &m, std::unique_ptr<Gaze> &result)
{
    while (1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        {
            Lock lk(m);
            for (int i : m) {
                std::cout << i;
                if (i == 10) {
                    std::cout << std::endl;
                    result.reset(new int(42));
                    return;
                }
            }
            std::cout << std::endl;
        }
    }
}

int main()
{
    Safe<vector<int>> m;
    std::unique_ptr<Gaze> result;
    std::thread reader(reader_thread, std::ref(m), std::ref(result));
    std::cout << "Started reader." << std::endl;
    for (int i=0; not result; ++i) {
        {
            Lock lk(m);
            m.push_back(i);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "Reader returned " << *result << std::endl;
    reader.join();
    return 0;
}
