#pragma once

#include "steppable/number.hpp"
#include "stpTypeName.hpp"

#include <any>

namespace steppable::parser
{
    std::unique_ptr<STP_TypeID> determineBinaryOperationFeasibility(STP_TypeID lhsType,
                                                              const std::string& operatorStr,
                                                              STP_TypeID rhsType);

    std::any performBinaryOperation(STP_TypeID lhsType,
                              std::any value,
                              std::string operatorStr,
                              STP_TypeID rhsType,
                              std::any rhsValue);

    std::any performUnaryOperation(STP_TypeID type, const std::string& operatorString, const std::any& value);
} // namespace steppable::parser