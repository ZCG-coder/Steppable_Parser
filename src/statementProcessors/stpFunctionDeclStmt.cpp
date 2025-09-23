#include "stpInterp/stpBetterTS.hpp"
#include "stpInterp/stpExprHandler.hpp"
#include "stpInterp/stpInit.hpp"
#include "stpInterp/stpProcessor.hpp"

namespace steppable::parser
{
    void STP_processFuncDefinition(const TSNode* node, const STP_InterpState& state)
    {
        TSNode fnNameNode = ts_node_child_by_field_name(*node, "fn_name"s);
        std::string fnName = state->getChunk(&fnNameNode);

        const TSNode posArgsNode = ts_node_next_named_sibling(fnNameNode);
        TSNode keywordArgsNode{};

        std::vector<std::string> posArgNames;
        STP_StringValMap keywordArgs;

        if (not ts_node_is_null(posArgsNode))
        {
            const size_t posArgsCount = ts_node_named_child_count(posArgsNode);

            for (size_t i = 0; i < posArgsCount; i++)
            {
                TSNode paramNode = ts_node_named_child(posArgsNode, i);
                std::string paramName = state->getChunk(&paramNode);
                posArgNames.emplace_back(paramName);
            }

            keywordArgsNode = ts_node_next_named_sibling(posArgsNode);
        }
        if (not ts_node_is_null(keywordArgsNode))
        {
            const size_t keywordArgsCount = ts_node_named_child_count(posArgsNode);

            for (size_t i = 0; i < keywordArgsCount; i++)
            {
                TSNode paramNode = ts_node_named_child(keywordArgsNode, i);
                TSNode paramNameNode = ts_node_named_child(paramNode, 0);
                TSNode paramValueNode = ts_node_named_child(paramNode, 1);

                STP_Value defaultVal = handleExpr(&paramValueNode, state);
                std::string paramName = state->getChunk(&paramNameNode);
                keywordArgs.insert_or_assign(paramName, defaultVal);
            }
        }

        TSNode bodyNode = ts_node_child_by_field_name(*node, "fn_body"s);

        STP_FunctionDefinition fn;
        fn.interpFn = [&](const STP_StringValMap& map) -> STP_Value {
            STP_Scope scope = state->addChildScope();

            scope.variables = map;
            // default return value
            scope.addVariable("04795", STP_Value(STP_TypeID_NUMBER, Number()));

            state->setCurrentScope(std::make_shared<STP_Scope>(scope));

            STP_processChunkChild(bodyNode, state, false);
            STP_Value ret = state->getCurrentScope()->getVariable("04795");
            state->setCurrentScope(state->getCurrentScope()->parentScope);

            return ret;
        };
        fn.posArgNames = posArgNames;
        fn.keywordArgs = keywordArgs;

        state->getCurrentScope()->addFunction(fnName, fn);
    }
}