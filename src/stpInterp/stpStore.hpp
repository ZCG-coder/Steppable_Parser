#pragma once
#include "../../../include/steppable/stpTypeName.hpp"
#include "fn/calc.hpp"
#include "steppable/stpArgSpace.hpp"

extern "C" {
#include <tree_sitter/api.h>
}

#include <any>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @namespace steppable::parser
 * @brief The parser for Steppable code.
 */
namespace steppable::parser
{
    class STP_DynamicLibrary
    {
    public:
        explicit STP_DynamicLibrary(const std::string& path);
        ~STP_DynamicLibrary();

        STP_ExportFuncT getSymbol(const std::string& name);

        bool isLoaded() const;

    private:
        void* handle;
    };

    struct STP_Value : STP_ValuePrimitive
    {
        [[nodiscard]] STP_Value applyBinaryOperator(const std::string& operatorStr, const STP_Value& rhs) const;

        [[nodiscard]] STP_Value applyUnaryOperator(const std::string& operatorStr) const;

        [[nodiscard]] bool asBool() const;

        explicit STP_Value(const STP_TypeID& type, const std::any& data = {}) : STP_ValuePrimitive(type, data) {}
    };

    using STP_StringValMap = std::unordered_map<std::string, STP_Value>;

    struct STP_FunctionDefinition
    {
        std::vector<std::string> posArgNames;

        STP_StringValMap keywordArgs;

        std::function<STP_Value(STP_StringValMap)> interpFn;

        STP_Value operator()(const STP_StringValMap& args) const { return interpFn(args); }
    };

    struct STP_Scope
    {
        STP_StringValMap variables;

        std::map<std::string, STP_FunctionDefinition> functions;

        std::shared_ptr<STP_Scope> parentScope = nullptr;

        void addVariable(const std::string& name, const STP_Value& data);

        STP_Value getVariable(const std::string& name);

        void addFunction(const std::string& name, const STP_FunctionDefinition& fn);

        STP_FunctionDefinition getFunction(const std::string& name);
    };

    class STP_InterpStoreLocal
    {
        STP_Scope globalScope;

        std::shared_ptr<STP_Scope> currentScope = std::make_shared<STP_Scope>(globalScope);

        std::string chunk;
        size_t chunkStart = 0;
        size_t chunkEnd = 0;

        std::string file = "/dev/null";

        std::vector<STP_DynamicLibrary> loadedLibraries;

    public:
        STP_InterpStoreLocal();

        void setChunk(const std::string& newChunk, const size_t& chunkStart, const size_t& chunkEnd);
        std::string getChunk(const TSNode* node = nullptr);

        [[nodiscard]] STP_Scope addChildScope(std::shared_ptr<STP_Scope> parent = nullptr) const;

        void setCurrentScope(std::shared_ptr<STP_Scope> newScope);

        auto getCurrentScope() { return currentScope; }

        auto getGlobalScope() { return std::make_shared<STP_Scope>(globalScope); };

        void setFile(const std::string& newFile);

        [[nodiscard]] std::string getFile() const;

        std::unique_ptr<STP_DynamicLibrary> getLoadedLib(const size_t& count)
        {
            if (count > loadedLibraries.size())
                return nullptr;
            return std::make_unique<STP_DynamicLibrary>(loadedLibraries[count]);
        }
    };
} // namespace steppable::parser
