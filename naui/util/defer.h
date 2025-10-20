#pragma once
#include "../base.h"
#include <tuple>

typedef void (*NauiDeferFunction)(void* args);

void naui_defer_internal(NauiDeferFunction func, void* args);
void naui_process_deferred();

NAUI_API void naui_flush_defer();

template<typename Fn, typename Tuple, size_t... I>
inline void naui_invoke_tuple(Fn fn, Tuple& t, std::index_sequence<I...>) 
{
    (void)fn(std::get<I>(t)...);
}

template<typename Fn, typename... Args>
inline void naui_defer(Fn fn, Args... args) 
{
    using Tuple = std::tuple<Fn, Args...>;
    auto* data = new Tuple(fn, args...);

    auto adapter = [](void* user) {
        Tuple* t = static_cast<Tuple*>(user);
        auto& f = std::get<0>(*t);

        // Extract arguments (skip the function itself)
        auto args_tuple = std::apply([&](auto&& f_, auto&&... rest) 
		{
            return std::make_tuple(rest...);
        }, *t);

        naui_invoke_tuple(f, args_tuple, std::make_index_sequence<sizeof...(Args)>{});
        delete t;
    };

    naui_defer_internal(adapter, data);
}