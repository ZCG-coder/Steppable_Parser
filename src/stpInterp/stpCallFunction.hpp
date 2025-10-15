/**************************************************************************************************
 * Copyright (c) 2023-2025 NWSOFT                                                                 *
 *                                                                                                *
 * Permission is hereby granted, free of charge, to any person obtaining a copy                   *
 * of this software and associated documentation files (the "Software"), to deal                  *
 * in the Software without restriction, including without limitation the rights                   *
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell                      *
 * copies of the Software, and to permit persons to whom the Software is                          *
 * furnished to do so, subject to the following conditions:                                       *
 *                                                                                                *
 * The above copyright notice and this permission notice shall be included in all                 *
 * copies or substantial portions of the Software.                                                *
 *                                                                                                *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR                     *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,                       *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE                    *
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER                         *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,                  *
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE                  *
 * SOFTWARE.                                                                                      *
 **************************************************************************************************/

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