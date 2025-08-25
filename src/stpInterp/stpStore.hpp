#pragma once

extern "C" {
#include <tree_sitter/api.h>
}

#include <any>
#include <map>
#include <memory>
#include <string>
#include <utility>

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
    extern "C" {
    constexpr char* const STP_typeNames[] = {
        [STP_TypeID_NUMBER] = const_cast<char* const>("Number"),
        [STP_TypeID_MATRIX_2D] = const_cast<char* const>("Mat2D"),
        [STP_TypeID_STRING] = const_cast<char* const>("Str"),
        [STP_TypeID_FUNC] = const_cast<char* const>("Func"),
        [STP_TypeID_OBJECT] = const_cast<char* const>("Object"),
        [STP_TypeID_NULL] = const_cast<char* const>("Null"),
    };
    }
    // NOLINTEND(cppcoreguidelines-pro-type-const-cast, cppcoreguidelines-avoid-c-arrays)

    struct STP_LocalValue
    {
        std::string typeName;
        STP_TypeID typeID;

        std::any data;

        explicit STP_LocalValue(const STP_TypeID& type, std::any data = {}) :
            typeName(STP_typeNames[type]), typeID(type), data(std::move(data))
        {
        }

        [[nodiscard]] std::string present() const;

        STP_LocalValue applyOperator(const std::string& operatorStr, const STP_LocalValue& rhs);
    };

    struct STP_Function
    {
        std::map<std::string, STP_LocalValue> args;

        STP_TypeID returnType;
    };

    struct STP_Scope
    {
        std::map<std::string, STP_LocalValue> variables;

        std::map<std::string, STP_Function> functions;

        std::shared_ptr<STP_Scope> parentScope = nullptr;

        void addVariable(const std::string& name, const STP_LocalValue& data);

        STP_LocalValue getVariable(const std::string& name);
    };

    class STP_InterpStoreLocal
    {
        size_t currentScope = 0;

        std::map<size_t, STP_Scope> scopes = {
            { 0, STP_Scope() },
        };

        std::string chunk;

    public:
        size_t chunkStart = 0;
        size_t chunkEnd = 0;

        STP_InterpStoreLocal();

        void addVariable(const std::string& name, const STP_LocalValue& typeID);

        STP_LocalValue getVariable(const std::string& name);

        void setScopeLevel(size_t newScope);

        size_t getScopeLevel() const;

        auto getScopes() -> decltype(scopes) const;

        void setChunk(const std::string& newChunk, size_t chunkStart, size_t chunkEnd);
        bool isChunkFull(const TSNode* node) const;
        std::string getChunk(const TSNode* node = nullptr);

        void dbgPrintVariables();
    };
} // namespace steppable::parser
