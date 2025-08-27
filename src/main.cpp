#include "output.hpp"
#include "platform.hpp"
#include "steppable/mat2d.hpp"
#include "steppable/mat2dBase.hpp"
#include "steppable/number.hpp"
#include "stpInterp/stpBetterTS.hpp"
#include "stpInterp/stpErrors.hpp"
#include "stpInterp/stpInit.hpp"
#include "stpInterp/stpStore.hpp"

#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

extern "C" {
#include <tree_sitter/api.h>
}

using namespace std::literals;
using namespace steppable::parser;
using namespace steppable::__internals;

constexpr long chunkSize = 1024L * 1024L; // 1MB
constexpr long overlap = 4096L; // 4KB

extern "C" TSLanguage* tree_sitter_stp();

std::string readFileChunk(const std::string& path, const long offset, bool& eof)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (not file)
        return "";

    file.seekg(offset, std::ios::beg);
    std::string chunk(chunkSize + overlap, '\0');
    file.read(chunk.data(), chunkSize + overlap);
    std::streamsize bytesRead = file.gcount();

    if (bytesRead < chunkSize + overlap)
    {
        chunk.resize(bytesRead);
        eof = true;
    }
    else
    {
        eof = false;
    }
    return chunk;
}

// void handleMemberAccess(TSNode* maNode, STP_InterpState state) {}

bool STP_isPureType(const std::string_view& str)
{
    return str == "matrix" or str == "number" or str == "string" or str == "identifier";
}

STP_LocalValue handleExpr(TSNode* exprNode, const STP_InterpState& state, const bool printResult = false)
{
    assert(exprNode != nullptr);
    if (ts_node_type(*exprNode) == "ERROR"s or ts_node_is_missing(*exprNode))
        STP_throwSyntaxError(*exprNode, state);

    std::string exprType = ts_node_type(*exprNode);

    STP_LocalValue retVal(STP_TypeID_NULL);

    if (exprType == "number")
    {
        // Number
        std::string data = state->getChunk(exprNode);
        retVal = STP_LocalValue(STP_TypeID_NUMBER, steppable::Number(data));
    }
    if (exprType == "percentage")
    {
        // percentage := number "%"
        TSNode numberNode = ts_node_child(*exprNode, 0);
        std::string number = state->getChunk(&numberNode);
        steppable::Number value(number);
        value /= 100; // NOLINT(*-avoid-magic-numbers)

        retVal = STP_LocalValue(STP_TypeID_NUMBER, value);
    }
    if (exprType == "matrix")
    {
        // Matrix
        std::unique_ptr<size_t> lastColLength;
        steppable::MatVec2D<steppable::Number> matVec;
        for (size_t j = 0; j < ts_node_child_count(*exprNode); j++)
        {
            TSNode node = ts_node_child(*exprNode, j);
            if (ts_node_type(node) != "matrix_row"s)
                continue;

            size_t currentCols = 0;
            std::vector<steppable::Number> currentMatRow;
            for (size_t i = 0; i < ts_node_child_count(node); i++)
            {
                TSNode cell = ts_node_child(node, i);
                if (ts_node_type(cell) == ";"s)
                    continue;
                STP_LocalValue val = handleExpr(&cell, state);
                if (val.typeID != STP_TypeID_NUMBER)
                {
                    steppable::output::error("parser"s, "Matrix should contain numbers only."s);
                    utils::programSafeExit(1);
                }
                auto value = std::any_cast<steppable::Number>(val.data);
                currentMatRow.emplace_back(value);
                currentCols++;
            }

            if (lastColLength)
            {
                if (currentCols != *lastColLength)
                {
                    steppable::output::error("parser"s, "Inconsistent matrix dimensions."s);
                    utils::programSafeExit(1);
                }
            }
            matVec.emplace_back(currentMatRow);
            lastColLength = std::make_unique<size_t>(currentCols);
        }

        steppable::Matrix data(matVec);
        retVal = STP_LocalValue(STP_TypeID_MATRIX_2D, data);
    }
    if (exprType == "string")
    {
        // String
        TSNode stringCharsNode = ts_node_child_by_field_name(*exprNode, "string_chars"s);
        std::string data = state->getChunk(&stringCharsNode);
        retVal = STP_LocalValue(STP_TypeID_STRING, data);
    }
    else if (exprType == "identifier")
    {
        // Get the variable
        std::string nameNode = state->getChunk(exprNode);
        retVal = state->getVariable(nameNode);
    }
    else if (exprType == "function_call")
    {
    }
    else
    {
        // Type is not specified by notation, need to infer.
        if (exprType == "binary_expression")
        {
            // binary_expression := lhs 'operator' rhs
            TSNode lhsNode = ts_node_child(*exprNode, 0);
            TSNode operandNode = ts_node_child(ts_node_child(*exprNode, 1), 0);
            TSNode rhsNode = ts_node_child(*exprNode, 2);

            std::string operandType = ts_node_type(operandNode);

            STP_LocalValue lhs = handleExpr(&lhsNode, state);
            STP_LocalValue rhs = handleExpr(&rhsNode, state);

            retVal = lhs.applyOperator(operandType, rhs);
        }
        if (exprType == "unary_expression")
        {
        }
        if (exprType == "bracketed_expr")
        {
            TSNode innerExpr = ts_node_child(*exprNode, 1);
            retVal = handleExpr(&innerExpr, state);
        }
    }

    if (printResult)
        std::cout << retVal.present() << '\n';

    return retVal;
}

void handleAssignment(TSNode* node, const STP_InterpState& state = nullptr)
{
    // assignment := nameNode "=" exprNode
    TSNode nameNode = ts_node_child(*node, 0);
    TSNode exprNode = ts_node_child(*node, 2);

    std::string name = state->getChunk(&nameNode);

    STP_LocalValue val = handleExpr(&exprNode, state);
    // Write to scope / global variables
    state->addVariable(name, val);
}

void processChunk(TSNode node, const std::string& chunk, int indent = 0, const STP_InterpState& state = nullptr)
{
    if (ts_node_type(node) == "ERROR"s or ts_node_is_missing(node))
        STP_throwSyntaxError(node, state);

    size_t childCount = 0;
    std::string text;
    std::string type = ts_node_type(node);

    // Ensure whole block is fully read
    if (not state->isChunkFull(&node))
        return;

    text = state->getChunk(&node);

    if (type == "\n" // Newline
        or type == "comment" // Comment
    )
    {
        return;
    }

    // Handle scoped statements before assignment statements
    if (type == "function_definition")
    {
        // std::string fnName = ;
        return;
    }
    if (type == "if_else_stmt")
    {
        // if_else_stmt := 'if'      expr '{' if_clause_stmt '}'
        //                 'else if' expr '{' statement '}'
        //                 'else'         '{' else_clause_stmt '}'
        TSNode exprNode = ts_node_named_child(node, 0);
        STP_LocalValue res = handleExpr(&exprNode, state);
        TSNode ifClauseStmtNode = ts_node_next_named_sibling(exprNode);
        TSNode lastNode = ifClauseStmtNode;

        if (res.asBool())
        {
            childCount = ts_node_child_count(ifClauseStmtNode);
            for (uint32_t i = 0; i < childCount; ++i)
                processChunk(ts_node_child(ifClauseStmtNode, i), chunk, indent + 1, state);
            return;
        }

        if (ts_node_is_null(lastNode))
        {
            // Don't go further if there is no elseif and else statements
            return;
        }

        while (true)
        {
            TSNode elseifClauseNode = ts_node_next_named_sibling(lastNode);
            if (ts_node_is_null(elseifClauseNode))
                break;
            if (ts_node_type(elseifClauseNode) != "elseif_clause"s)
                break;
            lastNode = elseifClauseNode;

            TSNode elseifExprNode = ts_node_named_child(elseifClauseNode, 0);
            TSNode elseifClauseStmtNode = ts_node_named_child(elseifClauseNode, 1);
            res = handleExpr(&elseifExprNode, state);

            if (res.asBool())
            {
                childCount = ts_node_child_count(elseifClauseStmtNode);
                for (uint32_t i = 0; i < childCount; ++i)
                    processChunk(ts_node_child(elseifClauseStmtNode, i), chunk, indent + 1, state);
                return;
            }
        }

        // Handle else statement
        if (ts_node_is_null(lastNode))
        {
            // If there is no else statement, skip it
            return;
        }

        TSNode elseClauseNode = ts_node_next_named_sibling(lastNode);
        TSNode elseClauseStmtNode = ts_node_named_child(elseClauseNode, 0);
        childCount = ts_node_child_count(elseClauseStmtNode);
        for (uint32_t i = 0; i < childCount; ++i)
            processChunk(ts_node_child(elseClauseStmtNode, i), chunk, indent + 1, state);
        return;
    }
    if (type == "while_stmt")
    {
        TSNode exprNode = ts_node_next_named_sibling(node);
        std::cout << ts_node_type(exprNode) << "\n";
        return;
    }
    if (type == "foreach_in_stmt")
    {
        //
        return;
    }
    if (type == "assignment")
    {
        handleAssignment(&node, state);
        return;
    }
    if (type == "expression_statement")
    {
        TSNode exprNode = ts_node_child(node, 0);
        handleExpr(&exprNode, state, true);
        return;
    }
    if (type == "import_statement")
    {
        //
        return;
    }
    if (type == "symbol_decl_statement")
    {
        TSNode nameNode = ts_node_child_by_field_name(node, "sym_name"s);
        const std::string& name = state->getChunk(&nameNode);

        STP_LocalValue assignmentVal(STP_TypeID_SYMBOL);
        assignmentVal.data = name;
        assignmentVal.typeName = STP_typeNames[STP_TypeID_SYMBOL];
        state->addVariable(name, assignmentVal);
        return;
    }

    if (type != "source_file")
    {
        for (int i = 0; i < indent - 1; ++i)
            std::cout << "    ";
        std::cout << "'" << type << "' : \"" << text << "\"\n";
    }

    childCount = ts_node_child_count(node);
    for (uint32_t i = 0; i < childCount; ++i)
        processChunk(ts_node_child(node, i), chunk, indent + 1, state);
}

int main(int argc, char** argv) // NOLINT(*-exception-escape)
{
    if (argc < 2)
    {
        std::cerr << "Usage: stp_parse <source-file>\n";
        return 1;
    }

    const std::string path = argv[1];
    long offset = 0;
    bool eof = false;
    TSParser* parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_stp());
    TSTree* tree = nullptr;

    const STP_InterpState state = STP_getState();
    state->setFile(path);

    while (not eof)
    {
        std::string chunk = readFileChunk(path, offset, eof);
        if (chunk.empty())
            break;

        if (tree != nullptr)
            ts_tree_delete(tree);

        tree = ts_parser_parse_string(parser, nullptr, chunk.c_str(), chunk.size());

        size_t chunkStart = offset;
        size_t chunkEnd = offset + chunkSize;

        const TSNode rootNode = ts_tree_root_node(tree);
        state->setChunk(chunk, chunkStart, chunkEnd);
        processChunk(rootNode, chunk, 0, state);

        offset += chunkSize;
    }

    if (tree != nullptr)
        ts_tree_delete(tree);
    ts_parser_delete(parser);

    state->dbgPrintVariables();
    STP_destroy();

    return 0;
}
