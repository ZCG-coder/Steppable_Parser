#include "include/stpStore.hpp"

#include <any>
#include <map>
#include <memory>
#include <string>
#include <sstream>

/**
 * @namespace steppable::parser
 * @brief The parser for Steppable code.
 */
namespace steppable::parser
{
    std::string STP_LocalValue::present() const
    {
        std::stringstream ret;

        ret << "(" << typeName << ") ";

        return ret.str();
    }

    void STP_Scope::addVariable(const std::string& name, const STP_TypeID& typeID, const std::any& data)
    {
        if (variables.find(name) == variables.end())
        {
            // variable exists
            return;
        }

        STP_LocalValuePtr value = std::make_shared<STP_LocalValue>();
        value->data = data;
        value->typeName = STP_typeNames[typeID]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        variables[name] = value;
    }

    STP_LocalValuePtr STP_Scope::getVariable(const std::string& name)
    {
        if (variables.find(name) == variables.end())
            return nullptr;
        return variables[name];
    }

    void STP_InterpStoreLocal::addVariable(const std::string& name, const STP_TypeID& typeID, const std::any& data)
    {
        STP_Scope scope = scopes[currentScope];
        scope.addVariable(name, typeID, data);
    }

    std::any STP_InterpStoreLocal::getVariable(const std::string& name)
    {
        if (scopes.find(currentScope) == scopes.end())
            return nullptr;
        STP_Scope scope = scopes[currentScope];
        return scope.getVariable(name);
    }

    void STP_InterpStoreLocal::setChunk(const std::string& newChunk, size_t chunkStart, size_t chunkEnd)
    {
        chunk = newChunk;
        this->chunkStart = chunkStart;
        this->chunkEnd = chunkEnd;
    }

    bool STP_InterpStoreLocal::isChunkFull(const TSNode* node)
    {
        uint32_t start = ts_node_start_byte(*node);
        uint32_t end = ts_node_end_byte(*node);
        return start < chunkStart or end > chunkEnd;
    }

    std::string STP_InterpStoreLocal::getChunk(const TSNode* node)
    {
        if (node == nullptr)
            return chunk;

        uint32_t start = ts_node_start_byte(*node);
        uint32_t end = ts_node_end_byte(*node);
        std::string text;
        if (end - chunkStart <= chunk.size())
            text = chunk.substr(start - chunkStart, end - start);
        return text;
    }

    void STP_InterpStoreLocal::setScopeLevel(size_t newScope) { currentScope = newScope; }

    size_t STP_InterpStoreLocal::getScopeLevel() { return currentScope; }

    void STP_InterpStoreLocal::dbgPrintVariables() {}
} // namespace steppable::parser
