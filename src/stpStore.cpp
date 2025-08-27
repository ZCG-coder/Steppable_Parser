#include "stpInterp/stpStore.hpp"

#include "steppable/mat2d.hpp"
#include "steppable/number.hpp"
#include "stpInterp/stpApplyOperator.hpp"
#include "util.hpp"

#include <any>
#include <map>
#include <memory>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

/**
 * @namespace steppable::parser
 * @brief The parser for Steppable code.
 */
namespace steppable::parser
{
    STP_LocalValue::STP_LocalValue(const STP_TypeID& type, std::any data) :
        typeName(STP_typeNames[type]), typeID(type), data(std::move(data))
    {
    }

    std::string STP_LocalValue::present() const
    {
        std::stringstream ret;
        std::string presented;
        std::string line;

        ret << "(" << typeName << ")";

        switch (typeID)
        {
        case STP_TypeID_NUMBER:
            presented = std::any_cast<Number>(data).present();
            break;
        case STP_TypeID_MATRIX_2D:
            presented = std::any_cast<Matrix>(data).present();
            break;
        case STP_TypeID_STRING:
            presented = "\"" + std::any_cast<std::string>(data) + "\"";
            break;
        default:
            break;
        }

        std::istringstream iss(presented);
        ret << "\n";
        while (getline(iss, line))
            ret << "    " << line << "\n";

        return ret.str();
    }

    STP_LocalValue STP_LocalValue::applyOperator(const std::string& _operatorStr, const STP_LocalValue& rhs) const
    {
        std::string operatorStr = _operatorStr;
        operatorStr = __internals::stringUtils::bothEndsReplace(operatorStr, ' ');

        STP_TypeID lhsType = this->typeID;
        STP_TypeID rhsType = rhs.typeID;
        STP_TypeID retType = *determineOperationFeasibility(lhsType, operatorStr, rhsType);

        std::any value = this->data;
        std::any rhsValue = rhs.data;

        STP_LocalValue returnVal(STP_TypeID_NULL);
        returnVal.typeID = retType;
        returnVal.typeName = STP_typeNames[retType];

        std::any returnValueAny = performOperation(lhsType, value, operatorStr, rhsType, rhsValue);

        if (not returnValueAny.has_value())
        {
            output::error("parser"s, "This operation is not supported at present"s);
            __internals::utils::programSafeExit(1);
        }
        returnVal.data = returnValueAny;

        return returnVal;
    }

    bool STP_LocalValue::asBool() const
    {
        switch (typeID)
        {
        case STP_TypeID_NULL:
            return false;
        case STP_TypeID_NUMBER:
        {
            const auto val = std::any_cast<Number>(data);
            return val != 0;
        }
        case STP_TypeID_MATRIX_2D:
        {
            const auto val = std::any_cast<Matrix>(data);
            return std::ranges::all_of(val.getData(), [](const std::vector<Number>& vec) {
                return std::ranges::all_of(vec, [](const Number& n) { return n != 0; });
            });
        }
        case STP_TypeID_STRING:
        {
            const auto val = std::any_cast<std::string>(data);
            return not val.empty();
        }
        case STP_TypeID_FUNC:
        {
            output::error("parser"s, "Cannot convert Func to a logical type"s);
            __internals::utils::programSafeExit(1);
        }
        case STP_TypeID_SYMBOL:
        {
            output::error("parser"s, "Cannot convert Symbol to a logical type"s);
            __internals::utils::programSafeExit(1);
        }
        }

        // Should not reach this
        return false;
    }

    void STP_Scope::addVariable(const std::string& name, const STP_LocalValue& data)
    {
        if (variables.contains(name))
        {
            // variable exists
            return;
        }

        variables.emplace(name, data);
    }

    STP_LocalValue STP_Scope::getVariable(const std::string& name)
    {
        if (not variables.contains(name))
        {
            if (parentScope == nullptr)
            {
                output::error("parser"s, "Variable {0} is not defined"s, { name });
                __internals::utils::programSafeExit(1);
            }
            return parentScope->getVariable(name);
        }
        return variables.at(name);
    }

    STP_InterpStoreLocal::STP_InterpStoreLocal() { setScopeLevel(0); }

    void STP_InterpStoreLocal::addVariable(const std::string& name, const STP_LocalValue& data)
    {
        STP_Scope& scope = scopes[currentScope];
        scope.addVariable(name, data);
    }

    STP_LocalValue STP_InterpStoreLocal::getVariable(const std::string& name)
    {
        if (not scopes.contains(currentScope))
        {
            output::error("parser"s, "Variable `{0}` is not defined"s, { name });
            __internals::utils::programSafeExit(1);
        }
        STP_Scope scope = scopes[currentScope];
        return scope.getVariable(name);
    }

    void STP_InterpStoreLocal::setScopeLevel(const size_t& newScope, const size_t& oldScope)
    {
        if (not scopes.contains(newScope))
        {
            // Create the scope
            STP_Scope scope;
            scope.parentScope = std::make_shared<STP_Scope>(scopes[oldScope]);
            scopes.emplace(newScope, scope);
        }
        currentScope = newScope;
    }

    size_t STP_InterpStoreLocal::getScopeLevel() const { return currentScope; }

    auto STP_InterpStoreLocal::getScopes() -> decltype(scopes) const { return scopes; }

    void STP_InterpStoreLocal::setChunk(const std::string& newChunk, const size_t& chunkStart, const size_t& chunkEnd)
    {
        chunk = newChunk;
        this->chunkStart = chunkStart;
        this->chunkEnd = chunkEnd;
    }

    bool STP_InterpStoreLocal::isChunkFull(const TSNode* node) const
    {
        uint32_t start = ts_node_start_byte(*node);
        uint32_t end = ts_node_end_byte(*node);
        return start >= chunkStart and end <= chunkEnd;
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

    void STP_InterpStoreLocal::dbgPrintVariables()
    {
        for (const auto& [scopeName, scope] : scopes)
        {
            std::cout << "In scope " << scopeName << "\n";
            for (const auto& var : scope.variables | std::views::values)
                std::cout << var.present() << "\n";
        }
    }

    void STP_InterpStoreLocal::setFile(const std::string& newFile) { file = newFile; }

    std::string STP_InterpStoreLocal::getFile() const { return file; }
} // namespace steppable::parser
