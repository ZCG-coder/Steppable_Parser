#pragma once

#include <any>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace steppable::parser
{
    // ReSharper disable once CppUnnamedNamespaceInHeaderFile
    namespace
    {
        template<typename... Args, std::size_t... I>
        void invokeFunction(const std::vector<std::any>& vec, void (*func)(Args...), std::index_sequence<I...>)
        {
            func(std::any_cast<Args>(vec[I])...);
        }
    }

    template<typename... Args>
    void STP_callAny(const std::vector<std::any>& vec, void (*func)(Args...))
    {
        if (vec.size() != sizeof...(Args))
            throw std::runtime_error("Argument count mismatch");
        invokeFunction(vec, func, std::index_sequence_for<Args...>{});
    }
} // namespace steppable::parser