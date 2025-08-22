#pragma once

#include <any>
#include <cstdint>
#include <map>
#include <string>

/**
 * @namespace steppable::parser
 * @brief The parser for Steppable code.
 */
namespace steppable::parser
{
    enum class TypeID : std::uint8_t
    {
        NUMBER = 0xDA,
        MATRIX_2D = 0xDB,
        STRING = 0xDC,
    };

    struct LocalValue
    {
        std::string typeName;
        TypeID typeID;

        std::string dataJSON;

        std::any data;
    };

    using VariablePair = std::map<std::string, LocalValue>;

    class STP_InterpStoreLocal
    {
        VariablePair globalVariables;

        std::map<size_t, VariablePair> scopedVariables;

    public:
        int addVariable(const std::string& name);

        void dbgPrintVariables();
    };
} // namespace steppable::parser
