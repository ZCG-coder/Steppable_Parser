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

#include "output.hpp"
#include "platform.hpp"
#include "replxx.hxx"
#include "stpInterp/stpErrors.hpp"
#include "stpInterp/stpInit.hpp"
#include "stpInterp/stpProcessor.hpp"
#include "tree_sitter/api.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <sstream>

using namespace std::literals;

namespace steppable::parser
{
    using Replxx = replxx::Replxx;
    using namespace replxx::color;
    using namespace std::placeholders;

    using ColorMap = std::map<std::string, Replxx::Color>;
    static const ColorMap STP_colorMap = { { "keyword", Replxx::Color::MAGENTA },
                                           { "number", Replxx::Color::YELLOW },
                                           { "variable", Replxx::Color::BLUE },
                                           { "function", Replxx::Color::CYAN },
                                           { "comment", Replxx::Color::BROWN },
                                           { "string", Replxx::Color::GREEN },
                                           { "string.escape", Replxx::Color::BRIGHTGREEN } };
    static const std::vector<std::string> queries = {
        R"(
        ; keywords
        ("fn") @keyword
        ("ret") @keyword
        ("sym") @keyword
        ("break") @keyword
        ("cont") @keyword
        ("exit") @keyword
        ("if") @keyword
        ("elseif") @keyword
        ("else") @keyword
        ("while") @keyword
        ("for") @keyword
        ("in") @keyword
        ("import") @keyword
        )",  R"((number) @number)",
        R"R(
        ; comment
        (comment) @comment
        )R",
        R"R(
        ; assignment
        (assignment
            name: (identifier) @variable)

        ; function params
        (fn_keyword_arg
            argument_name: (param_name) @variable
        )
        (fn_pos_arg
            argument_name: (param_name) @variable
        )
        )R",
        R"R(
        ; function call
        (function_call
            fn_name: (identifier) @function
        )

        ; function definition
        (function_definition
            fn_name: (function_name) @function
        )
        )R",
        R"R(
        (escape_sequence) @string.escape
        (unicode_escape) @string.escape
        (octal_escape) @string.escape
        )R",
        R"R(
        (string_char) @string
        )R",
    };

    void hookColor(std::string const& context,
                   Replxx::colors_t& colors,
                   const ColorMap& colorMap,
                   TSParser* parser,
                   TSQueryCursor* cursor)
    {
        colors.assign(context.length(), replxx::Replxx::Color::DEFAULT); // Default color for all characters

        TSQueryMatch match;
        uint32_t captureIdx = 0;
        const TSLanguage* language = ts_parser_language(parser);
        uint32_t errOffset = 0;
        TSQueryError errType{};

        TSTree* tree = ts_parser_parse_string(parser, nullptr, context.c_str(), static_cast<uint32_t>(context.size()));

        for (const auto& querySource : queries)
        {
            TSQuery* query = ts_query_new(language, querySource.c_str(), querySource.length(), &errOffset, &errType);
            ts_query_cursor_exec(cursor, query, ts_tree_root_node(tree));
            while (ts_query_cursor_next_capture(cursor, &match, &captureIdx))
            {
                for (uint16_t i = 0; i < match.capture_count; i++)
                {
                    TSNode node = match.captures[i].node;
                    uint32_t start = ts_node_start_byte(node);
                    uint32_t end = ts_node_end_byte(node);

                    uint32_t captureNameLen = 0;
                    const char* captureName = ts_query_capture_name_for_id(query, captureIdx, &captureNameLen);
                    std::string captureNameS(captureName, captureName + captureNameLen);
                    Replxx::Color color = Replxx::Color::DEFAULT;
                    if (STP_colorMap.contains(captureNameS))
                        color = STP_colorMap.at(captureNameS);

                    for (uint32_t i = start; i < end; i++)
                        colors[i] = color;
                }
            }
            ts_query_delete(query);
        }

        ts_tree_delete(tree);
    }

    int STP_startInteractiveMode(const STP_InterpState& state, TSParser* parser)
    {
        std::cout << "Steppable " STEPPABLE_VERSION "\n";
        std::cout << "Written by Andy Zhang, licensed under MIT license (C) 2023-2025.\n";
        std::cout << "* For more information, type `license()`.\n";
        std::cout << "* Type an expression to evaluate. `exit` to quit.\n";

        std::string source;
        TSTree* tree = nullptr;

        uint32_t errorOffset = 0;
        TSQueryError errorType{};
        TSQueryCursor* cursor = ts_query_cursor_new();
        const TSLanguage* language = ts_parser_language(parser);

        Replxx rx;
        rx.set_highlighter_callback([&](auto&& PH1, auto&& PH2) {
            hookColor(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2), STP_colorMap, parser, cursor);
        });

        while (true)
        {
            source.clear();
            source = rx.input(": ");

            tree = ts_parser_parse_string(parser, nullptr, source.c_str(), static_cast<uint32_t>(source.size()));
            uint32_t errorOffset = 0;
            TSQueryError errorType;

            TSNode rootNode = ts_tree_root_node(tree);
            state->setChunk(source, 0, static_cast<long>(source.size()));
            if (STP_checkRecursiveNodeSanity(rootNode, state))
                continue;

            STP_processChunkChild(rootNode, state);
        }
        return 0;
    }
} // namespace steppable::parser
