#pragma once

template<typename Function>
struct FunctionTraits;

template <typename Ret, typename... Args>
struct FunctionTraits<Ret(Args...)> {
    typedef Ret(*ptr)(Args...);
};

template <typename Ret, typename... Args>
struct FunctionTraits<Ret(*const)(Args...)> : FunctionTraits<Ret(Args...)> {};

template <typename Cls, typename Ret, typename... Args>
struct FunctionTraits<Ret(Cls::*)(Args...) const> : FunctionTraits<Ret(Args...)> {};


template <typename F>
auto lambdaToPointer(F lambda) -> typename FunctionTraits<decltype(&F::operator())>::ptr {
    static auto lambda_copy = lambda; // this little trick works, because every lambda is different anonymous class

    return []<typename... Args>(Args... args) {
        return lambda_copy(args...);
    };
}

