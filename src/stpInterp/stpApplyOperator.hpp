#pragma once

#include "steppable/number.hpp"
#include "stpTypeName.hpp"

#include <any>

namespace steppable::parser
{
    std::unique_ptr<STP_TypeID> determineOperationFeasibility(STP_TypeID lhsType,
                                                              const std::string& operatorStr,
                                                              STP_TypeID rhsType);

    std::any performOperation(STP_TypeID lhsType,
                              std::any value,
                              std::string operatorStr,
                              STP_TypeID rhsType,
                              std::any rhsValue);
} // namespace steppable::parser