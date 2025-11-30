#pragma once
#include "../base.h"
#include <functional>
#include <vector>

namespace Naui {

class Defer {
public:
    template<typename Fn, typename... Args>
    static void Add(Fn&& fn, Args&&... args) {
        callbacks.push_back([=]() {
            fn(args...);
        });
    }

    static void Process() {
        for (auto& cb : callbacks) {
            cb();
        }

        callbacks.clear();
    }

    static void Flush() {
        callbacks.clear();
    }

private:
    inline static std::vector<std::function<void()>> callbacks;
};

}