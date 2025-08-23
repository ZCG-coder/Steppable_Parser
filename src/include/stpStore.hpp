#pragma once

#include "tree_sitter/api.h"

#include <any>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>

/**
 * @namespace steppable::parser
 * @brief The parser for Steppable code.
 */
namespace steppable::parser
{
    enum STP_TypeID : std::uint8_t
    {
        STP_TypeID_NULL = 0,
        STP_TypeID_NUMBER = 1,
        STP_TypeID_MATRIX_2D = 2,
        STP_TypeID_STRING = 3,
        STP_TypeID_FUNC = 4,
        STP_TypeID_OBJECT = 5,
    };

    // NOLINTBEGIN(cppcoreguidelines-pro-type-const-cast, cppcoreguidelines-avoid-c-arrays)
    constexpr char* const STP_typeNames[] = {
        [STP_TypeID_NUMBER] = const_cast<char* const>("Number"),
        [STP_TypeID_MATRIX_2D] = const_cast<char* const>("Mat2D"),
        [STP_TypeID_STRING] = const_cast<char* const>("Str"),
        [STP_TypeID_FUNC] = const_cast<char* const>("Func"),
        [STP_TypeID_OBJECT] = const_cast<char* const>("Object"),
        [STP_TypeID_NULL] = const_cast<char* const>("Null"),
    };
    // NOLINTEND(cppcoreguidelines-pro-type-const-cast, cppcoreguidelines-avoid-c-arrays)

    struct STP_LocalValue
    {
        std::string typeName;
        STP_TypeID typeID;

        std::any data;

        std::string present() const;
    };

    using STP_LocalValuePtr = std::shared_ptr<STP_LocalValue>;

    struct STP_Scope
    {
        std::map<std::string, STP_LocalValuePtr> variables;

        std::shared_ptr<STP_Scope> parentScope = nullptr;

        void addVariable(const std::string& name, const STP_TypeID& typeID, const std::any& data);

        STP_LocalValuePtr getVariable(const std::string& name);
    };

    class STP_InterpStoreLocal
    {
        size_t currentScope;

        std::map<size_t, STP_Scope> scopes;

        std::string chunk;

    public:
        size_t chunkStart;
        size_t chunkEnd;

        void addVariable(const std::string& name, const STP_TypeID& typeID, const std::any& data);

        std::any getVariable(const std::string& name);

        void setScopeLevel(size_t newScope);

        size_t getScopeLevel();

        void setChunk(const std::string& newChunk, size_t chunkStart, size_t chunkEnd);
        bool isChunkFull(const TSNode* node);
        std::string getChunk(const TSNode* node = nullptr);

        void dbgPrintVariables();
    };
} // namespace steppable::parser
