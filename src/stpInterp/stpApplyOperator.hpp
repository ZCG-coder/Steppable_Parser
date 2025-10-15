#pragma once

#include "../../../include/steppable/stpTypeName.hpp"
#include "steppable/number.hpp"
#include "tree_sitter/api.h"

#include <any>

namespace steppable::parser
{
    std::unique_ptr<STP_TypeID> determineBinaryOperationFeasibility(const TSNode* node,
                                                                    STP_TypeID lhsType,
                                                                    const std::string& operatorStr,
                                                                    STP_TypeID rhsType);

    std::any performBinaryOperation(const TSNode* node,
                                    STP_TypeID lhsType,
                                    std::any value,
                                    std::string operatorStr,
                                    STP_TypeID rhsType,
                                    std::any rhsValue);

    std::any performUnaryOperation(const TSNode* node,
                                   STP_TypeID type,
                                   const std::string& operatorString,
                                   const std::any& value);
} // namespace steppable::parser