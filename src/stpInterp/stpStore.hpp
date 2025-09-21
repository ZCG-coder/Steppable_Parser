#pragma once
#include "../../../include/steppable/stpTypeName.hpp"
#include "fn/calc.hpp"

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

    struct STP_LocalValue
    {
        std::string typeName;
        STP_TypeID typeID;

        std::any data;

        explicit STP_LocalValue(const STP_TypeID& type, std::any data = {});

        [[nodiscard]] std::string present(const std::string& name, bool longFormat = true) const;

        [[nodiscard]] STP_LocalValue applyBinaryOperator(const std::string& operatorStr,
                                                         const STP_LocalValue& rhs) const;

        [[nodiscard]] STP_LocalValue applyUnaryOperator(const std::string& operatorStr) const;

        [[nodiscard]] bool asBool() const;
    };

    using STP_StringValMap = std::unordered_map<std::string, STP_LocalValue>;

    struct STP_FunctionDefinition
    {
        std::vector<std::string> argNames;

        std::function<void(STP_StringValMap)> interpFn;

        void operator()(const STP_StringValMap& args) const { interpFn(args); }
    };

    struct STP_Scope
    {
        STP_StringValMap variables;

        std::map<std::string, STP_FunctionDefinition> functions;

        std::shared_ptr<STP_Scope> parentScope = nullptr;

        void addVariable(const std::string& name, const STP_LocalValue& data);

        STP_LocalValue getVariable(const std::string& name);
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
