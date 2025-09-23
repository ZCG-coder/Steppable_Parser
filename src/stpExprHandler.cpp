#include "steppable/mat2d.hpp"
#include "steppable/number.hpp"
#include "steppable/stpArgSpace.hpp"
#include "stpInterp/stpBetterTS.hpp"
#include "stpInterp/stpErrors.hpp"
#include "stpInterp/stpInit.hpp"
#include "stpInterp/stpStore.hpp"

#include <cassert>

namespace steppable::parser
{
    std::vector<STP_Argument> extractArgVector(const TSNode* exprNode, const STP_InterpState& state);

    STP_Value handleExpr(const TSNode* exprNode,
                         const STP_InterpState& state,
                         const bool printResult = false,
                         const std::string& exprName = "")
    {
        assert(exprNode != nullptr);
        if (ts_node_type(*exprNode) == "ERROR"s or ts_node_is_missing(*exprNode))
            STP_throwSyntaxError(*exprNode, state);

        std::string exprType = ts_node_type(*exprNode);

        STP_Value retVal(STP_TypeID_NULL);

        if (exprType == "number")
        {
            // Number
            std::string data = state->getChunk(exprNode);
            retVal = STP_Value(STP_TypeID_NUMBER, Number(data));
        }
        if (exprType == "percentage")
        {
            // percentage := number "%"
            TSNode numberNode = ts_node_child(*exprNode, 0);
            std::string number = state->getChunk(&numberNode);
            Number value(number);
            value /= 100; // NOLINT(*-avoid-magic-numbers)

            retVal = STP_Value(STP_TypeID_NUMBER, value);
        }
        if (exprType == "matrix")
        {
            // Matrix
            std::unique_ptr<size_t> lastColLength;
            MatVec2D<Number> matVec;
            for (size_t j = 0; j < ts_node_child_count(*exprNode); j++)
            {
                TSNode node = ts_node_child(*exprNode, j);
                if (const std::string& nodeType = ts_node_type(node);
                    nodeType != "matrix_row" and nodeType != "matrix_row_last")
                    continue;

                size_t currentCols = 0;
                std::vector<Number> currentMatRow;
                for (size_t i = 0; i < ts_node_child_count(node); i++)
                {
                    TSNode cell = ts_node_child(node, i);
                    if (ts_node_type(cell) == ";"s)
                        continue;
                    STP_Value val = handleExpr(&cell, state);
                    if (val.typeID != STP_TypeID_NUMBER)
                    {
                        output::error("parser"s, "Matrix should contain numbers only."s);
                        programSafeExit(1);
                    }
                    auto value = std::any_cast<Number>(val.data);
                    currentMatRow.emplace_back(value);
                    currentCols++;
                }

                if (lastColLength)
                {
                    if (currentCols != *lastColLength)
                    {
                        output::error("parser"s, "Inconsistent matrix dimensions."s);
                        programSafeExit(1);
                    }
                }
                matVec.emplace_back(currentMatRow);
                lastColLength = std::make_unique<size_t>(currentCols);
            }

            Matrix data(matVec);
            retVal = STP_Value(STP_TypeID_MATRIX_2D, data);
        }
        if (exprType == "string")
        {
            using namespace __internals;
            // String
            std::string data;
            for (size_t i = 0; i < ts_node_child_count(*exprNode); i++)
            {
                auto childNode = ts_node_child(*exprNode, i);
                const std::string childNodeType = ts_node_type(childNode);

                if (childNodeType == "string_char")
                    data += state->getChunk(&childNode);
                else if (childNodeType == "unicode_escape")
                {
                    auto hexDigitsNode = ts_node_named_child(childNode, 0);
                    const std::string hexCode = state->getChunk(&hexDigitsNode);

                    unsigned long codePoint = std::stoul(hexCode, nullptr, 16);
                    std::string text = stringUtils::unicodeToUtf8(static_cast<int>(codePoint));
                    data += text;
                }
                else if (childNodeType == "octal_escape")
                {
                    std::string octDigits = state->getChunk(&childNode);
                    octDigits.erase(octDigits.begin()); // Erase leading '\' character

                    unsigned long codePoint = std::stoul(octDigits, nullptr, 8);
                    std::string text = stringUtils::unicodeToUtf8(static_cast<int>(codePoint));
                    data += text;
                }
                else if (childNodeType == "formatting_snippet")
                {
                    auto formatExprNode = ts_node_child_by_field_name(childNode, "formatting_expr"s);
                    STP_Value value = handleExpr(&formatExprNode, state, printResult, exprName);

                    data += value.present("", false);
                }
            }
            retVal = STP_Value(STP_TypeID_STRING, data);
        }
        else if (exprType == "identifier_or_member_access")
        {
            TSNode childNode = ts_node_child(*exprNode, 0);
            std::string childNodeType = ts_node_type(childNode);

            // Get the variable
            if (childNodeType == "identifier")
            {
                std::string nameNode = state->getChunk(&childNode);
                retVal = state->getCurrentScope()->getVariable(nameNode);
            }
        }
        else if (exprType == "function_call")
        {
            TSNode nameNode = ts_node_child(ts_node_child(*exprNode, 0), 0);

            if (ts_node_type(nameNode) == "identifier"s)
            {
                std::string funcNameOrig = state->getChunk(&nameNode);
                std::string funcName = "STP_" + funcNameOrig;

                auto stpLib = state->getLoadedLib(0);
                auto funcPtr = stpLib->getSymbol(funcName);

                auto functionsVec = state->getCurrentScope()->functions;
                bool executed = false;

                if (funcPtr == nullptr)
                {
                    if (functionsVec.contains(funcNameOrig))
                    {
                        // call from Steppable-defined functions
                        auto function = functionsVec[funcNameOrig];
                        std::vector<STP_Argument> fnArgsVec = extractArgVector(exprNode, state);

                        std::vector<STP_Argument> posArgs;
                        std::vector<STP_Argument> keywordArgs;

                        STP_StringValMap declaredKeywordArgs = function.keywordArgs;
                        STP_StringValMap givenKeywordArgs;

                        std::ranges::copy_if(fnArgsVec, std::back_inserter(posArgs), [](const STP_Argument& arg) {
                            return arg.name.empty();
                        });
                        std::ranges::copy_if(fnArgsVec, std::back_inserter(keywordArgs), [](const STP_Argument& arg) {
                            return not arg.name.empty();
                        });

                        std::ranges::transform(
                            keywordArgs, std::inserter(givenKeywordArgs, givenKeywordArgs.end()), [](const auto& pair) {
                                return std::make_pair(pair.name, STP_Value(pair.typeID, pair.value));
                            });

                        if (posArgs.size() != function.posArgNames.size())
                        {
                            std::vector<std::string> missingArgsNames;
                            std::copy(function.posArgNames.begin() + static_cast<ssize_t>(function.posArgNames.size()) -
                                          static_cast<ssize_t>(posArgs.size()),
                                      function.posArgNames.end(),
                                      missingArgsNames.begin());

                            output::error("runtime"s,
                                          "Missing positional arguments. Expect {0}"s,
                                          {
                                              __internals::stringUtils::join(missingArgsNames, ","s),
                                          });
                            programSafeExit(1);
                        }

                        STP_StringValMap argMap;
                        for (size_t i = 0; i < posArgs.size(); i++)
                        {
                            std::string posArgName = function.posArgNames[i];
                            const STP_Argument& currentArg = posArgs[i];
                            argMap.insert_or_assign(posArgName, STP_Value(currentArg.typeID, currentArg.value));
                        }
                        argMap.merge(declaredKeywordArgs);
                        argMap.merge(givenKeywordArgs);

                        retVal = function.interpFn(argMap);
                        executed = true;
                    }
                    else
                    {
                        output::error("runtime"s, "Function {0} is not defined."s, { funcNameOrig });
                        programSafeExit(1);
                    }
                }

                if (not executed)
                {
                    std::vector<STP_Argument> fnArgsVec = extractArgVector(exprNode, state);

                    auto args = STP_ArgContainer(fnArgsVec, {});
                    auto* val = static_cast<STP_ValuePrimitive*>(funcPtr(&args));

                    retVal = STP_Value(val->typeID, val->data);
                }
            }
            // Member access function
        }
        else
        {
            // Type is not specified by notation, need to infer.
            if (exprType == "binary_expression")
            {
                // binary_expression := lhs 'operator' rhs
                TSNode binExprNode = ts_node_child(*exprNode, 0);
                TSNode lhsNode = ts_node_child(binExprNode, 0);
                TSNode operandNode = ts_node_child(ts_node_child(binExprNode, 1), 0);
                TSNode rhsNode = ts_node_child(binExprNode, 2);

                std::string operandType = ts_node_type(operandNode);

                STP_Value lhs = handleExpr(&lhsNode, state);
                STP_Value rhs = handleExpr(&rhsNode, state);

                retVal = lhs.applyBinaryOperator(operandType, rhs);
            }
            if (exprType == "unary_expression")
            {
                TSNode operandNode = ts_node_child(*exprNode, 0);
                TSNode child = ts_node_child(*exprNode, 1);
                std::string operandType = ts_node_type(operandNode);

                STP_Value childVal = handleExpr(&child, state);
                retVal = childVal.applyUnaryOperator(operandType);
            }
            if (exprType == "bracketed_expr")
            {
                TSNode innerExpr = ts_node_child(*exprNode, 1);
                retVal = handleExpr(&innerExpr, state);
            }
        }

        if (printResult)
            std::cout << retVal.present(exprName) << '\n';

        return retVal;
    }

    std::vector<STP_Argument> extractArgVector(const TSNode* exprNode, const STP_InterpState& state)
    {
        std::vector<STP_Argument> fnArgsVec;
        TSNode posArgumentsNode = ts_node_named_child(*exprNode, 1);
        if (not ts_node_is_null(posArgumentsNode))
        {
            size_t posArgumentsCount = ts_node_named_child_count(posArgumentsNode);

            for (size_t i = 0; i < posArgumentsCount; i++)
            {
                TSNode argNode = ts_node_named_child(posArgumentsNode, i);
                STP_Value res = handleExpr(&argNode, state);
                STP_Argument argument("", res.data, res.typeID);
                fnArgsVec.emplace_back(argument);
            }

            TSNode kwArgumentsNode = ts_node_named_child(*exprNode, 2);
            if (not ts_node_is_null(kwArgumentsNode))
            {
                size_t keywordArgumentCount = ts_node_named_child_count(kwArgumentsNode);
                for (size_t i = 0; i < keywordArgumentCount; i++)
                {
                    TSNode argNode = ts_node_named_child(kwArgumentsNode, i);
                    TSNode argNameNode = ts_node_child_by_field_name(argNode, "argument_name"s);
                    TSNode argExprNode = ts_node_next_named_sibling(argNameNode);

                    std::string argName = state->getChunk(&argNameNode);

                    STP_Value res = handleExpr(&argExprNode, state);
                    STP_Argument argument(argName, res.data, res.typeID);
                    fnArgsVec.emplace_back(argument);
                }
            }
        }

        return fnArgsVec;
    }

} // namespace steppable::parser