#include "output.hpp"
#include "platform.hpp"
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

constexpr long chunkSize = 1024L * 1024L; // 1MB
constexpr long overlap = 4096L; // 4KB

extern "C" TSLanguage* tree_sitter_stp();

std::string readFileChunk(const std::string& path, long offset, long chunkSize, long overlap, bool& eof)
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

void handleMemberAccess(TSNode* maNode, STP_InterpState state) {}

bool STP_isPureType(const std::string_view& str)
{
    return str == "matrix" or str == "number" or str == "string" or str == "identifier";
}

STP_LocalValue handleExpr(TSNode* exprNode, STP_InterpState state)
{
    assert(exprNode != nullptr);

    std::string exprType = ts_node_type(*exprNode);

    STP_TypeID typeID = STP_TypeID_NULL;
    STP_LocalValue retVal(STP_TypeID_NULL);

    if (exprType == "number")
    {
        // Number
        typeID = STP_TypeID_NUMBER;
    }
    else if (exprType == "matrix")
    {
        // Matrix
        typeID = STP_TypeID_MATRIX_2D;
        size_t rows = 0;
        size_t lastColLength = 0;
        for (size_t j = 0; j < ts_node_child_count(*exprNode); j++)
        {
            TSNode node = ts_node_child(*exprNode, j);
            if (ts_node_type(node) != "matrix_row"s)
                continue;

            size_t currentCols = 0;
            for (size_t i = 0; i < ts_node_child_count(node); i++)
            {
                TSNode cell = ts_node_child(node, i);
                if (ts_node_type(node) == "matrix_row"s)
                    break;
                STP_LocalValue val = handleExpr(&cell, state);
                if (val.typeID != STP_TypeID_NUMBER)
                {
                    steppable::output::error("parser"s, "Matrix should contain numbers only."s);
                    steppable::__internals::utils::programSafeExit(1);
                }
                currentCols++;
            }

            rows++;
            if (currentCols != lastColLength)
            {
                steppable::output::error("parser"s, "Inconsistent matrix dimensions."s);
                steppable::__internals::utils::programSafeExit(1);
            }
            lastColLength = currentCols;
        }

        TSNode firstNode = ts_node_child(*exprNode, 1);
        size_t cols = ts_node_child_count(firstNode);

        std::cout << rows << " " << cols << std::endl;
    }
    else if (exprType == "string")
    {
        // String
        typeID = STP_TypeID_STRING;
        std::string data = state->getChunk(exprNode);
        std::cout << data << "\n";
        retVal.data = data;
    }
    else if (exprType == "identifier")
    {
        // Get the variable
        std::string nameNode = state->getChunk(exprNode);
        return state->getVariable(nameNode);
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
            std::string lhsType = ts_node_type(lhsNode);
            std::string rhsType = ts_node_type(rhsNode);

            STP_LocalValue lhs = handleExpr(&lhsNode, state);
            STP_LocalValue rhs = handleExpr(&rhsNode, state);
            std::cout << lhs.present() << operandType << rhs.present() << '\n';

            return lhs.applyOperator(operandType, rhs);
        }
        if (exprType == "unary_expression")
        {
        }
        if (exprType == "bracketed_expr")
        {
            TSNode innerExpr = ts_node_child(*exprNode, 1);
            return handleExpr(&innerExpr, state);
        }
    }

    std::string typeName = STP_typeNames[typeID];
    retVal.typeID = typeID;
    retVal.typeName = typeName;

    return retVal;
}

void handleAssignment(TSNode* node, STP_InterpState state = nullptr)
{
    // assignment := nameNode "=" exprNode
    TSNode nameNode = ts_node_child(*node, 0); // type is identifer or member access
    TSNode exprNode = ts_node_child(*node, 2);
    std::string nameNodeType = ts_node_type(nameNode);

    if (nameNodeType == "identifier")
    {
        std::string name = state->getChunk(&nameNode);
        std::cout << name << "\n";

        STP_LocalValue val = handleExpr(&exprNode, state);
        // Write to scope / global variables
        state->addVariable(name, val);
    }
    else if (nameNodeType == "member_access")
    {
    }
}

void processChunk(TSNode node, const std::string& chunk, int indent = 0, STP_InterpState state = nullptr)
{
    uint32_t child_count = 0;
    std::string text;
    std::string type = ts_node_type(node);

    // Ensure whole block is fully read
    if (state->isChunkFull(&node))
        return;

    text = state->getChunk(&node);

    if (type == "\n" // Newline
        or type == "comment" // Comment
        or type == "(" or type == ")" // Parantheses
        or type == "[" or type == "]" // Square brackets
        or type == "{" or type == "}" // Braces
        or type == "=" // Assignment
        or type == "->" // Return type
        or type == "," //
        or type == "." // Member access
        or type == ";" // Force statement break
        or type == "\"" // String quotes
        or type == "|" // Matrix sep
        or type == "\\{" or type == "\\}" // String formatting
        or type == "\\x" // String Unicode escape
        or type == "^" // Binary operators
        or type == "&" // Binary operators
        or type == "*" // Binary operators
        or type == "/" // Binary operators
        or type == "-" // Binary operators / Unary operator
        or type == "+" // Binary operators / Unary operator
        or type == "==" // Binary operators
        or type == "!=" // Binary operators
        or type == ">" // Binary operators
        or type == "<" // Binary operators
        or type == ">=" // Binary operators
        or type == "<=" // Binary operators
        or type == ".*" // Binary operators
        or type == "./" // Binary operators
        or type == ".^" // Binary operators
        or type == "%" // Percentages
        or type == "!" // Unary not
    )
    {
        return;
    }

    for (int i = 0; i < indent - 1; ++i)
        std::cout << "    ";
    std::cout << "'" << type << "' : \"" << text << "\"\n";

    // Handle scoped statements before assignment statements
    if (type == "function_definition")
    {
        //
        return;
    }
    if (type == "object_definition")
    {
        //
        return;
    }
    if (type == "if_else_stmt")
    {
        //
        return;
    }
    if (type == "while_stmt")
    {
        //
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
        //
        TSNode exprNode = ts_node_child(node, 0);
        handleExpr(&exprNode, state);
        return;
    }
    if (type == "import_statement")
    {
        //
        return;
    }

    child_count = ts_node_child_count(node);
    for (uint32_t i = 0; i < child_count; ++i)
        processChunk(ts_node_child(node, i), chunk, indent + 1, state);
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: stp_parse <source-file>\n";
        return 1;
    }

    std::string path = argv[1];
    long offset = 0;
    bool eof = false;
    TSParser* parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_stp());
    TSTree* tree = nullptr;

    STP_InterpState state = STP_getState();

    while (not eof)
    {
        std::string chunk = readFileChunk(path, offset, chunkSize, overlap, eof);
        if (chunk.empty())
            break;

        if (tree != nullptr)
            ts_tree_delete(tree);

        tree = ts_parser_parse_string(parser, nullptr, chunk.c_str(), chunk.size());

        size_t chunkStart = offset;
        size_t chunkEnd = offset + chunkSize;

        TSNode rootNode = ts_tree_root_node(tree);
        state->setChunk(chunk, chunkStart, chunkEnd);
        processChunk(rootNode, chunk, 0, state);

        offset += chunkSize;
    }

    if (tree != nullptr)
        ts_tree_delete(tree);
    ts_parser_delete(parser);

    STP_destroy();

    return 0;
}
