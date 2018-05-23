#ifndef CEAGLE_STRINGUTIL_H
#define CEAGLE_STRINGUTIL_H

#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <future>

using std::string;
using std::vector;

namespace ceagle {

namespace util {

class UltilityTimer {
public:
    template <class callable, class... arguments> UltilityTimer(int after, bool async, callable&& f, arguments&&... args) {
        std::function<typename std::result_of<callable(arguments...)>::type()> task(std::bind(std::forward<callable>(f), std::forward<arguments>(args)...));

        if (async) {
            std::thread([after, task]() {std::this_thread::sleep_for(std::chrono::seconds(after)); task();}).detach();
            //std::async([after, task](){std::this_thread::sleep_for(std::chrono::seconds(after)); task();});
        } else {
            std::this_thread::sleep_for(std::chrono::seconds(after));
            task();
        }
    }
};


// 20161214, experimental purpose
int randomBetween(int min, int max);
int useCPU();

int countSubstring(const string& str, const string& sub);
void replaceAll(string& str, const string& from, const string& to);
string getGraphmlStr(const string& ele, const string& content = "", const vector<string> properties = vector<string>());

}

}

#endif
