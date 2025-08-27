#pragma once
#include "stpTypeName.h"

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
    struct STP_LocalValue
    {
        std::string typeName;
        STP_TypeID typeID;

        std::any data;

        explicit STP_LocalValue(const STP_TypeID& type, std::any data = {});

        [[nodiscard]] std::string present(const std::string& name) const;

        [[nodiscard]] STP_LocalValue applyOperator(const std::string& operatorStr, const STP_LocalValue& rhs) const;

        [[nodiscard]] bool asBool() const;
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

        std::map<size_t, STP_Scope> scopes;

        std::string chunk;
        size_t chunkStart = 0;
        size_t chunkEnd = 0;

        std::string file = "/dev/null";

    public:
        STP_InterpStoreLocal();

        void addVariable(const std::string& name, const STP_LocalValue& typeID);

        STP_LocalValue getVariable(const std::string& name);

        STP_Function getFunction(const std::string& name);

        void setScopeLevel(const size_t& newScope, const size_t& oldScope = 0);

        [[nodiscard]] size_t getScopeLevel() const;

        auto getScopes() -> decltype(scopes) const;

        void setChunk(const std::string& newChunk, const size_t& chunkStart, const size_t& chunkEnd);
        bool isChunkFull(const TSNode* node) const;
        std::string getChunk(const TSNode* node = nullptr);

        void dbgPrintVariables();

        void setFile(const std::string& newFile);

        [[nodiscard]] std::string getFile() const;
    };
} // namespace steppable::parser
