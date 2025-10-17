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

#include "stpInterp/stpErrors.hpp"
#include "stpInterp/stpInit.hpp"
#include "stpInterp/stpProcessor.hpp"

namespace steppable::parser
{
    int STP_startInteractiveMode(const STP_InterpState& state, TSParser* parser)
    {
        std::cout << "Steppable " STEPPABLE_VERSION "\n";
        std::cout << "Written by Andy Zhang, licensed under MIT license (C) 2023-2025.\n";
        std::cout << "* For more information, type `license()`.\n";
        std::cout << "* Type an expression to evaluate. `exit` to quit.\n";

        std::string source;
        TSTree* tree = nullptr;

        while (true)
        {
            std::cout << ": ";
            source.clear();
            std::getline(std::cin, source);

            tree = ts_parser_parse_string(parser, nullptr, source.c_str(), static_cast<uint32_t>(source.size()));
            TSNode rootNode = ts_tree_root_node(tree);
            state->setChunk(source, 0, static_cast<long>(source.size()));
            if (STP_checkRecursiveNodeSanity(rootNode, state))
                continue;

            STP_processChunkChild(rootNode, state);
        }
        return 0;
    }
} // namespace steppable::parser