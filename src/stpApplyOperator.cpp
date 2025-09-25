#include "stpInterp/stpApplyOperator.hpp"

#include "steppable/mat2d.hpp"

namespace steppable::parser
{
    std::unique_ptr<STP_TypeID> determineBinaryOperationFeasibility(const STP_TypeID lhsType,
                                                                    const std::string& operatorStr,
                                                                    const STP_TypeID rhsType)
    {
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

        bool operationPerformable = false;
        STP_TypeID retType = STP_TypeID::NONE;

        // region Magic
        if ((lhsType == STP_TypeID::SYMBOL and rhsType != STP_TypeID::STRING) or
            (lhsType != STP_TypeID::STRING and rhsType == STP_TypeID::SYMBOL))
        {
            // Symbols operations are always possible
            operationPerformable = true;
            retType = STP_TypeID::SYMBOL;
        }
        else if (operatorStr == "+" or operatorStr == "-")
        {
            operationPerformable = (lhsType == STP_TypeID::NUMBER and rhsType == STP_TypeID::NUMBER) or
                                   (lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::MATRIX_2D) or
                                   (lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::NUMBER) or
                                   (lhsType == STP_TypeID::NUMBER and rhsType == STP_TypeID::MATRIX_2D) or
                                   (lhsType == STP_TypeID::STRING and rhsType == STP_TypeID::STRING);

            if (lhsType == STP_TypeID::NUMBER and rhsType == STP_TypeID::NUMBER)
                retType = STP_TypeID::NUMBER;
            else if ((lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::NUMBER) or
                     (rhsType == STP_TypeID::MATRIX_2D and lhsType == STP_TypeID::NUMBER) or
                     (lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::MATRIX_2D))
                retType = STP_TypeID::MATRIX_2D;
            else if (lhsType == STP_TypeID::STRING or rhsType == STP_TypeID::STRING)
                retType = STP_TypeID::STRING;
        }
        else if (operatorStr == "*" and ((lhsType == STP_TypeID::STRING and rhsType == STP_TypeID::NUMBER) or
                                         (lhsType == STP_TypeID::NUMBER and rhsType == STP_TypeID::STRING)))
        {
            operationPerformable = true;
            retType = STP_TypeID::STRING;
        }
        else if (operatorStr == "*" or operatorStr == "/")
        {
            operationPerformable = (lhsType == STP_TypeID::NUMBER and rhsType == STP_TypeID::NUMBER) or
                                   (lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::MATRIX_2D) or
                                   (lhsType == STP_TypeID::NUMBER and rhsType == STP_TypeID::MATRIX_2D) or
                                   (lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::NUMBER);

            if (lhsType == STP_TypeID::NUMBER and rhsType == STP_TypeID::NUMBER)
                retType = STP_TypeID::NUMBER;
            else if (lhsType == STP_TypeID::MATRIX_2D or rhsType == STP_TypeID::MATRIX_2D)
                retType = STP_TypeID::MATRIX_2D;
        }
        else if (operatorStr == ".*" or operatorStr == "./" or operatorStr == ".^" or operatorStr == "&")
        {
            operationPerformable = (lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::MATRIX_2D);
            retType = STP_TypeID::MATRIX_2D;
        }
        else if (operatorStr == "@")
        {
            operationPerformable = (lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::MATRIX_2D);
            retType = STP_TypeID::NUMBER;
        }
        else if (operatorStr == "^")
        {
            operationPerformable = (lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::MATRIX_2D) or
                                   (lhsType == STP_TypeID::NUMBER and rhsType == STP_TypeID::NUMBER) or
                                   (lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::NUMBER);
            if (lhsType == STP_TypeID::MATRIX_2D)
                retType = STP_TypeID::MATRIX_2D;
            else
                retType = STP_TypeID::NUMBER;
        }
        else if (operatorStr == "==" or operatorStr == "!=" or operatorStr == ">" or operatorStr == "<" or
                 operatorStr == ">=" or operatorStr == "<=")
        {
            operationPerformable = lhsType == rhsType;
            if (lhsType == STP_TypeID::MATRIX_2D or rhsType == STP_TypeID::MATRIX_2D)
                retType = STP_TypeID::MATRIX_2D;
            else
                retType = STP_TypeID::NUMBER;
        }
        else if (operatorStr == "mod")
        {
            operationPerformable = (lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::MATRIX_2D) or
                                   (lhsType == STP_TypeID::NUMBER and rhsType == STP_TypeID::NUMBER) or
                                   (lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::NUMBER);
            if (lhsType == STP_TypeID::MATRIX_2D)
                retType = STP_TypeID::MATRIX_2D;
            else
                retType = STP_TypeID::NUMBER;
        }
        // endregion

        if (not operationPerformable)
        {
            output::error("parser"s,
                          "Operation ({0}) {1} ({2}) cannot be performed."s,
                          { STP_typeNames.at(lhsType), operatorStr, STP_typeNames.at(rhsType) });
            __internals::utils::programSafeExit(1);
        }

        return std::make_unique<STP_TypeID>(retType);
    }

    std::unique_ptr<STP_TypeID> determineUnaryOperationFeasibility(const std::string& operatorString,
                                                                   const STP_TypeID type)
    {
        bool operationPerformable = type == STP_TypeID::NUMBER or type == STP_TypeID::MATRIX_2D;
        STP_TypeID retType = type;

        if (operationPerformable)
            return std::make_unique<STP_TypeID>(retType);

        output::error(
            "parser"s, "Operation {0}({1}) cannot be performed."s, { operatorString, STP_typeNames.at(type) });
        __internals::utils::programSafeExit(1);
        return nullptr; // not reachable
    }

    std::any performBinaryOperation(STP_TypeID lhsType,
                                    std::any value,
                                    std::string operatorStr,
                                    STP_TypeID rhsType,
                                    std::any rhsValue)
    {
        std::any returnValueAny;
        if (operatorStr == "+")
        {
            if (lhsType == STP_TypeID::NUMBER and rhsType == STP_TypeID::NUMBER)
                returnValueAny = std::any_cast<Number>(value) + std::any_cast<Number>(rhsValue);
            else if (lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::MATRIX_2D)
                returnValueAny = Matrix(std::any_cast<Matrix>(value) + std::any_cast<Matrix>(rhsValue));
            else if (lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::NUMBER)
                returnValueAny = Matrix(std::any_cast<Matrix>(value) + std::any_cast<Number>(rhsValue));
            else if (lhsType == STP_TypeID::NUMBER and rhsType == STP_TypeID::MATRIX_2D)
                returnValueAny = Matrix(std::any_cast<Matrix>(rhsValue) + std::any_cast<Number>(value));
            else if (lhsType == STP_TypeID::STRING and rhsType == STP_TypeID::STRING)
                returnValueAny = std::any_cast<std::string>(value) + std::any_cast<std::string>(rhsValue);
        }
        else if (operatorStr == "-")
        {
            if (lhsType == STP_TypeID::NUMBER and rhsType == STP_TypeID::NUMBER)
                returnValueAny = std::any_cast<Number>(value) - std::any_cast<Number>(rhsValue);
            else if (lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::MATRIX_2D)
                returnValueAny = Matrix(std::any_cast<Matrix>(value) - std::any_cast<Matrix>(rhsValue));
            else if (lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::NUMBER)
                returnValueAny = Matrix(std::any_cast<Matrix>(value) - std::any_cast<Number>(rhsValue));
            else if (lhsType == STP_TypeID::NUMBER and rhsType == STP_TypeID::MATRIX_2D)
            {
                const auto rhsObj = std::any_cast<Matrix>(rhsValue);
                const auto lhsObjNumber = std::any_cast<Number>(value);
                const auto lhsObjMatrix = Matrix(rhsObj.size(), lhsObjNumber);
                returnValueAny = Matrix(lhsObjMatrix - rhsObj);
            }
        }
        else if (operatorStr == "*")
        {
            if (lhsType == STP_TypeID::NUMBER and rhsType == STP_TypeID::NUMBER)
                returnValueAny = std::any_cast<Number>(value) * std::any_cast<Number>(rhsValue);
            else if (lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::MATRIX_2D)
                returnValueAny = Matrix(std::any_cast<Matrix>(value) * std::any_cast<Matrix>(rhsValue));
            else if (lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::NUMBER)
                returnValueAny = Matrix(std::any_cast<Matrix>(value) * std::any_cast<Number>(rhsValue));
            else if (lhsType == STP_TypeID::NUMBER and rhsType == STP_TypeID::MATRIX_2D)
                returnValueAny = Matrix(std::any_cast<Matrix>(rhsValue) * std::any_cast<Number>(value));
            else if (lhsType == STP_TypeID::STRING and rhsType == STP_TypeID::NUMBER)
            {
                auto str = std::any_cast<std::string>(value);
                auto times = std::any_cast<Number>(rhsValue);
                std::string result;
                for (Number i = 0; i < times; ++i)
                    result += str;
                returnValueAny = result;
            }
        }
        else if (operatorStr == "/")
        {
            if (lhsType == STP_TypeID::NUMBER and rhsType == STP_TypeID::NUMBER)
                returnValueAny = std::any_cast<Number>(value) / std::any_cast<Number>(rhsValue);
            else if (lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::MATRIX_2D)
                returnValueAny = Matrix(std::any_cast<Matrix>(value) / std::any_cast<Matrix>(rhsValue));
            else if (lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::NUMBER)
                returnValueAny = Matrix(std::any_cast<Matrix>(value) / std::any_cast<Number>(rhsValue));
            // else if (lhsType == STP_TypeID::NUMBER and rhsType == STP_TypeID::MATRIX_2D)
            //     returnValueAny = std::any_cast<Number>(value) / std::any_cast<Matrix>(rhsValue);
        }
        else if (operatorStr == "mod")
        {
            if (lhsType == STP_TypeID::NUMBER and rhsType == STP_TypeID::NUMBER)
                returnValueAny = std::any_cast<Number>(value).mod(std::any_cast<Number>(rhsValue));
        }
        else if (operatorStr == "^")
        {
            if (lhsType == STP_TypeID::NUMBER and rhsType == STP_TypeID::NUMBER)
                returnValueAny = std::any_cast<Number>(value) ^ std::any_cast<Number>(rhsValue);
            else if (lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::NUMBER)
                returnValueAny = Matrix(std::any_cast<Matrix>(value) ^ std::any_cast<Number>(rhsValue));
            // else if (lhsType == STP_TypeID::NUMBER and rhsType == STP_TypeID::MATRIX_2D)
            //     /* Implementation pending */;
        }
        else if (operatorStr == "@")
        {
            if (lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::MATRIX_2D)
                returnValueAny = Matrix(std::any_cast<Matrix>(value).dot(std::any_cast<Matrix>(rhsValue)));
        }
        // else if (operatorStr == "&")
        // {
        //     if (lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::MATRIX_2D)
        //         returnValueAny = cross(std::any_cast<Matrix>(value), std::any_cast<Matrix>(rhsValue));
        // }
        else if (operatorStr == "==")
        {
            if (lhsType == STP_TypeID::NUMBER and rhsType == STP_TypeID::NUMBER)
                returnValueAny = Number(std::any_cast<Number>(value) == std::any_cast<Number>(rhsValue));
            else if (lhsType == STP_TypeID::STRING and rhsType == STP_TypeID::STRING)
                returnValueAny = Number(std::any_cast<std::string>(value) == std::any_cast<std::string>(rhsValue));
            else if (lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::MATRIX_2D)
                returnValueAny =
                    Number(static_cast<bool>(std::any_cast<Matrix>(value) == std::any_cast<Matrix>(rhsValue)));
            // else if (lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::NUMBER)
            //     returnValueAny = Number(std::any_cast<Matrix>(value) == std::any_cast<Number>(rhsValue));
        }
        else if (operatorStr == "!=")
        {
            if (lhsType == STP_TypeID::NUMBER and rhsType == STP_TypeID::NUMBER)
                returnValueAny = Number(std::any_cast<Number>(value) != std::any_cast<Number>(rhsValue));
            else if (lhsType == STP_TypeID::STRING and rhsType == STP_TypeID::STRING)
                returnValueAny = Number(std::any_cast<std::string>(value) != std::any_cast<std::string>(rhsValue));
            else if (lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::MATRIX_2D)
                returnValueAny =
                    Number(static_cast<bool>(std::any_cast<Matrix>(value) != std::any_cast<Matrix>(rhsValue)));
            // else if (lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::NUMBER)
            //     returnValueAny = Number(std::any_cast<Matrix>(value) != std::any_cast<Number>(rhsValue));
        }
        else if (operatorStr == ">")
        {
            if (lhsType == STP_TypeID::NUMBER and rhsType == STP_TypeID::NUMBER)
                returnValueAny = Number(std::any_cast<Number>(value) > std::any_cast<Number>(rhsValue));
            else if (lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::MATRIX_2D)
                returnValueAny = Matrix(std::any_cast<Matrix>(value) > std::any_cast<Matrix>(rhsValue));
            // else if (lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::NUMBER)
            //     returnValueAny = Number(std::any_cast<Matrix>(value) > std::any_cast<Number>(rhsValue));
        }
        else if (operatorStr == "<")
        {
            if (lhsType == STP_TypeID::NUMBER and rhsType == STP_TypeID::NUMBER)
                returnValueAny = Number(std::any_cast<Number>(value) < std::any_cast<Number>(rhsValue));
            else if (lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::MATRIX_2D)
                returnValueAny = Matrix(std::any_cast<Matrix>(value) < std::any_cast<Matrix>(rhsValue));
            // else if (lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::NUMBER)
            //     returnValueAny = Number(std::any_cast<Matrix>(value) < std::any_cast<Number>(rhsValue));
        }
        else if (operatorStr == ">=")
        {
            if (lhsType == STP_TypeID::NUMBER and rhsType == STP_TypeID::NUMBER)
                returnValueAny = Number(std::any_cast<Number>(value) >= std::any_cast<Number>(rhsValue));
            else if (lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::MATRIX_2D)
                returnValueAny = Matrix(std::any_cast<Matrix>(value) >= std::any_cast<Matrix>(rhsValue));
            // else if (lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::NUMBER)
            //     returnValueAny = Number(std::any_cast<Matrix>(value) >= std::any_cast<Number>(rhsValue));
        }
        else if (operatorStr == "<=")
        {
            if (lhsType == STP_TypeID::NUMBER and rhsType == STP_TypeID::NUMBER)
                returnValueAny = Number(std::any_cast<Number>(value) <= std::any_cast<Number>(rhsValue));
            else if (lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::MATRIX_2D)
                returnValueAny = Matrix(std::any_cast<Matrix>(value) <= std::any_cast<Matrix>(rhsValue));
            // else if (lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::NUMBER)
            //     returnValueAny = Number(std::any_cast<Matrix>(value) <= std::any_cast<Number>(rhsValue));
        }
        else if (operatorStr == ".*")
        {
            if (lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::MATRIX_2D)
            {
                auto lhs = std::any_cast<Matrix>(value);
                auto rhs = std::any_cast<Matrix>(rhsValue);
                auto result = lhs.elemWiseMultiply(rhs);
                returnValueAny = Matrix(result);
            }
        }
        else if (operatorStr == "./")
        {
            if (lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::MATRIX_2D)
                returnValueAny = Matrix(std::any_cast<Matrix>(value).elemWiseDivision(std::any_cast<Matrix>(rhsValue)));
        }
        else if (operatorStr == ".^")
        {
            if (lhsType == STP_TypeID::MATRIX_2D and rhsType == STP_TypeID::MATRIX_2D)
                returnValueAny = Matrix(std::any_cast<Matrix>(value).elemWisePower(std::any_cast<Matrix>(rhsValue)));
        }

        return returnValueAny;
    }

    std::any performUnaryOperation(STP_TypeID type, const std::string& operatorString, const std::any& value)
    {
        determineUnaryOperationFeasibility(operatorString, type);

        if (operatorString == "!")
        {
            if (type == STP_TypeID::MATRIX_2D)
            {
                const auto result = std::any_cast<Matrix>(value);
                return not result;
            }
            if (type == STP_TypeID::NUMBER)
            {
                const auto result = std::any_cast<Number>(value);
                return not result;
            }
            if (type == STP_TypeID::STRING)
            {
                const auto result = std::any_cast<std::string>(value);
                return not result.empty();
            }
        }
        else if (operatorString == "+")
        {
            if (type == STP_TypeID::MATRIX_2D)
            {
                const auto result = std::any_cast<Matrix>(value);
                return +result;
            }
            if (type == STP_TypeID::NUMBER)
            {
                const auto result = std::any_cast<Number>(value);
                return +result;
            }
        }
        else if (operatorString == "-")
        {
            if (type == STP_TypeID::MATRIX_2D)
            {
                const auto result = std::any_cast<Matrix>(value);
                return -result;
            }
            if (type == STP_TypeID::NUMBER)
            {
                const auto result = std::any_cast<Number>(value);
                return -result;
            }
        }

        return std::make_any<std::nullptr_t>(nullptr);
    }
} // namespace steppable::parser