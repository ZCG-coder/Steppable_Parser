#include "stpInterp/stpStore.hpp"

#include "util.hpp"

#include <any>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

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

    STP_LocalValue STP_LocalValue::applyOperator(const std::string& _operatorStr, const STP_LocalValue& rhs)
    {
        std::string operatorStr = _operatorStr;
        operatorStr = __internals::stringUtils::bothEndsReplace(operatorStr, ' ');

        STP_TypeID lhsType = this->typeID;
        STP_TypeID rhsType = rhs.typeID;

        bool operationPerformable = false;
        STP_TypeID retType = STP_TypeID_NULL;

        // Make sure the operation can be performed
        //
        // Operator     lhsType        rhsType      retType             Desc
        // + -          number         number       number              Simple add / subtract
        // + -          matrix         matrix       matrix              Matrix add / subtract
        // + -          matrix         number       matrix              Implicit broadcast add / subtract
        // + -          number         matrix       matrix              :
        //
        // * /          number         matrix       matrix              Implicit broadcast multiply / division
        // * /          matrix         number       matrix              :
        // * /          matrix         matrix       matrix              Matrix multiplication / Multiply with M^-1
        // * /          number         number       number              Simple multiply / division
        //
        // .*           matrix         matrix       matrix              In-place multiplication
        // ./           matrix         matrix       matrix              In-place division
        // .^           matrix         matrix       matrix              In-place power
        //
        // mod          matrix         matrix       matrix              Modulus
        // mod          number         number       number              Simple modulus
        // mod          matrix         number       matrix              In-place modulus
        //
        //
        // @            matrix         matrix       number              Dot product
        // &            matrix         matrix       matrix              Cross product
        //
        // +            string         string       string              String concat.
        // *            string         number       string              String repeat
        //
        // ^            number         number       number              Simple exponential
        // ^            matrix         number       matrix              Matrix power
        // ^            number         matrix       number              * Implementation pending
        //
        // == != > <    any            any          number (0, 1)       If lhs and rhs are number / string
        // <= >=                                    matrix              If lhs or rhs is matrix
        //                                                              lhs and rhs must be the same type.
        //
        // [fn call]    any            any          any                 Depends on implementation

        if (operatorStr == "+" or operatorStr == "-")
        {
            operationPerformable = (lhsType == STP_TypeID_NUMBER and rhsType == STP_TypeID_NUMBER) or
                                   (lhsType == STP_TypeID_MATRIX_2D and rhsType == STP_TypeID_MATRIX_2D) or
                                   (lhsType == STP_TypeID_MATRIX_2D and rhsType == STP_TypeID_NUMBER) or
                                   (lhsType == STP_TypeID_NUMBER and rhsType == STP_TypeID_MATRIX_2D) or
                                   (lhsType == STP_TypeID_STRING and rhsType == STP_TypeID_STRING);

            if (lhsType == STP_TypeID_NUMBER and rhsType == STP_TypeID_NUMBER)
                retType = STP_TypeID_NUMBER;
            else if ((lhsType == STP_TypeID_MATRIX_2D and rhsType == STP_TypeID_NUMBER) or
                     (rhsType == STP_TypeID_MATRIX_2D and lhsType == STP_TypeID_NUMBER) or
                     (lhsType == STP_TypeID_MATRIX_2D and rhsType == STP_TypeID_MATRIX_2D))
                retType = STP_TypeID_MATRIX_2D;
            else if (lhsType == STP_TypeID_STRING or rhsType == STP_TypeID_STRING)
                retType = STP_TypeID_STRING;
        }
        else if (operatorStr == "*" and ((lhsType == STP_TypeID_STRING and rhsType == STP_TypeID_NUMBER) or
                                         (lhsType == STP_TypeID_NUMBER and rhsType == STP_TypeID_STRING)))
        {
            operationPerformable = true;
            retType = STP_TypeID_STRING;
        }
        else if (operatorStr == "*" or operatorStr == "/")
        {
            operationPerformable = (lhsType == STP_TypeID_NUMBER and rhsType == STP_TypeID_NUMBER) or
                                   (lhsType == STP_TypeID_MATRIX_2D and rhsType == STP_TypeID_MATRIX_2D) or
                                   (lhsType == STP_TypeID_NUMBER and rhsType == STP_TypeID_MATRIX_2D) or
                                   (lhsType == STP_TypeID_MATRIX_2D and rhsType == STP_TypeID_NUMBER);

            if (lhsType == STP_TypeID_NUMBER and rhsType == STP_TypeID_NUMBER)
                retType = STP_TypeID_NUMBER;
            else if (lhsType == STP_TypeID_MATRIX_2D or rhsType == STP_TypeID_MATRIX_2D)
                retType = STP_TypeID_MATRIX_2D;
        }
        else if (operatorStr == ".*" or operatorStr == "./" or operatorStr == ".^" or operatorStr == "&")
        {
            operationPerformable = (lhsType == STP_TypeID_MATRIX_2D and rhsType == STP_TypeID_MATRIX_2D);
            retType = STP_TypeID_MATRIX_2D;
        }
        else if (operatorStr == "@")
        {
            operationPerformable = (lhsType == STP_TypeID_MATRIX_2D and rhsType == STP_TypeID_MATRIX_2D);
            retType = STP_TypeID_NUMBER;
        }
        else if (operatorStr == "==" or operatorStr == "!=" or operatorStr == ">" or operatorStr == "<" or
                 operatorStr == ">=" or operatorStr == "<=")
        {
            operationPerformable = lhsType == rhsType;
            if (lhsType == STP_TypeID_MATRIX_2D or rhsType == STP_TypeID_MATRIX_2D)
                retType = STP_TypeID_MATRIX_2D;
            else
                retType = STP_TypeID_NUMBER;
        }
        else if (operatorStr == "mod")
        {
            operationPerformable = (lhsType == STP_TypeID_MATRIX_2D and rhsType == STP_TypeID_MATRIX_2D) or
                                   (lhsType == STP_TypeID_NUMBER and rhsType == STP_TypeID_NUMBER) or
                                   (lhsType == STP_TypeID_MATRIX_2D and rhsType == STP_TypeID_NUMBER);
            if (lhsType == STP_TypeID_MATRIX_2D)
                retType = STP_TypeID_MATRIX_2D;
            else
                retType = STP_TypeID_NUMBER;
        }

        if (not operationPerformable)
            throw std::runtime_error("Cannot perform this oepration.");

        std::any value = this->data;
        std::any rhsValue = rhs.data;

        STP_LocalValue returnVal(STP_TypeID_NULL);
        returnVal.typeID = retType;
        returnVal.typeName = STP_typeNames[retType];
        returnVal.data = value;

        return returnVal;
    }

    void STP_Scope::addVariable(const std::string& name, const STP_LocalValue& data)
    {
        if (not variables.contains(name))
        {
            // variable exists
            return;
        }

        variables.at(name) = data;
    }

    STP_LocalValue STP_Scope::getVariable(const std::string& name)
    {
        if (not variables.contains(name))
            return STP_LocalValue(STP_TypeID_NULL);
        return variables.at(name);
    }

    void STP_InterpStoreLocal::addVariable(const std::string& name, const STP_LocalValue& data)
    {
        STP_Scope scope = scopes[currentScope];
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

    void STP_InterpStoreLocal::setScopeLevel(size_t newScope) { currentScope = newScope; }

    size_t STP_InterpStoreLocal::getScopeLevel() const { return currentScope; }

    void STP_InterpStoreLocal::setChunk(const std::string& newChunk, size_t chunkStart, size_t chunkEnd)
    {
        chunk = newChunk;
        this->chunkStart = chunkStart;
        this->chunkEnd = chunkEnd;
    }

    bool STP_InterpStoreLocal::isChunkFull(const TSNode* node) const
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

    void STP_InterpStoreLocal::dbgPrintVariables() {}
} // namespace steppable::parser
