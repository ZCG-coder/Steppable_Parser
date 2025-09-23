#include "stpInterp/stpExprHandler.hpp"
#include "stpInterp/stpBetterTS.hpp"

namespace steppable::parser
{
    STP_Value STP_handleStringExpr(const TSNode* exprNode, const STP_InterpState& state)
    {
        using namespace __internals;
        // String
        std::string data;
        for (uint32_t i = 0; i < ts_node_child_count(*exprNode); i++)
        {
            auto childNode = ts_node_child(*exprNode, i);
            const std::string childNodeType = ts_node_type(childNode);

            if (childNodeType == "string_char")
                data += state->getChunk(&childNode);
            else if (childNodeType == "unicode_escape")
            {
                auto hexDigitsNode = ts_node_named_child(childNode, 0);
                const std::string hexCode = state->getChunk(&hexDigitsNode);

                unsigned long codePoint = std::stoul(hexCode, nullptr, 16);
                std::string text = stringUtils::unicodeToUtf8(static_cast<int>(codePoint));
                data += text;
            }
            else if (childNodeType == "octal_escape")
            {
                std::string octDigits = state->getChunk(&childNode);
                octDigits.erase(octDigits.begin()); // Erase leading '\' character

                unsigned long codePoint = std::stoul(octDigits, nullptr, 8);
                std::string text = stringUtils::unicodeToUtf8(static_cast<int>(codePoint));
                data += text;
            }
            else if (childNodeType == "formatting_snippet")
            {
                auto formatExprNode = ts_node_child_by_field_name(childNode, "formatting_expr"s);
                STP_Value value = handleExpr(&formatExprNode, state, false, "");

                data += value.present("", false);
            }
        }

        return STP_Value(STP_TypeID_STRING, data);
    }
}
