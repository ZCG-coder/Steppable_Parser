#include "stpInterp/stpStore.hpp"

#include "steppable/mat2d.hpp"
#include "steppable/number.hpp"
#include "stpInterp/stpApplyOperator.hpp"
#include "util.hpp"

#include <any>
#include <map>
#include <memory>
#include <ranges>
#include <string>
#include <utility>
#if defined(_WIN32)
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

/**
 * @namespace steppable::parser
 * @brief The parser for Steppable code.
 */
namespace steppable::parser
{
    STP_DynamicLibrary::STP_DynamicLibrary(const std::string& _path) : handle(nullptr)
    {
        std::string path = _path;
#if defined(WINDOWS)
        path += ".dll";
        handle = (void*)LoadLibraryA(path.c_str());
#elif defined(MACOSX)
        path = "lib" + path + ".dylib";
        handle = dlopen(path.c_str(), RTLD_LAZY);
#else
        path = "lib" + path + ".so";
        handle = dlopen(path.c_str(), RTLD_LAZY);
#endif
    }

    STP_DynamicLibrary::~STP_DynamicLibrary()
    {
#if defined(_WIN32)
        if (handle != nullptr)
            FreeLibrary((HMODULE)handle);
#else
        if (handle != nullptr)
            dlclose(handle);
#endif
    }

    STP_ExportFuncT STP_DynamicLibrary::getSymbol(const std::string& name)
    {
#if defined(_WIN32)
        if (handle != nullptr)
        {
            auto* ptr = reinterpret_cast<void* (*)(void*)>(GetProcAddress((HMODULE)handle, name.c_str()));
            return ptr;
        }
        return nullptr;
#else
        if (handle != nullptr)
        {
            auto* ptr = reinterpret_cast<STP_ExportFuncPtr*>( // NOLINT(*-pro-type-reinterpret-cast)
                dlsym(handle, name.c_str()));
            return ptr;
        }
        return nullptr;
#endif
    }

    bool STP_DynamicLibrary::isLoaded() const { return handle != nullptr; }

    STP_Value STP_Value::applyBinaryOperator(const std::string& _operatorStr, const STP_Value& rhs) const
    {
        std::string operatorStr = _operatorStr;
        operatorStr = __internals::stringUtils::bothEndsReplace(operatorStr, ' ');

        STP_TypeID lhsType = this->typeID;
        STP_TypeID rhsType = rhs.typeID;
        STP_TypeID retType = *determineBinaryOperationFeasibility(lhsType, operatorStr, rhsType);

        std::any value = this->data;
        std::any rhsValue = rhs.data;

        STP_Value returnVal(STP_TypeID_NULL);
        returnVal.typeID = retType;
        returnVal.typeName = STP_typeNames.at(retType);

        std::any returnValueAny = performBinaryOperation(lhsType, value, operatorStr, rhsType, rhsValue);

        if (not returnValueAny.has_value())
        {
            output::error("parser"s, "This operation is not supported at present"s);
            programSafeExit(1);
        }
        returnVal.data = returnValueAny;

        return returnVal;
    }

    STP_Value STP_Value::applyUnaryOperator(const std::string& _operatorStr) const
    {
        std::string operatorStr = _operatorStr;
        operatorStr = __internals::stringUtils::bothEndsReplace(operatorStr, ' ');

        std::any returnValAny = performUnaryOperation(typeID, operatorStr, data);

        STP_Value returnValue(typeID, returnValAny);
        return returnValue;
    }

    bool STP_Value::asBool() const
    {
        switch (typeID)
        {
        case STP_TypeID_NULL:
            return false;
        case STP_TypeID_NUMBER:
        {
            const auto val = std::any_cast<Number>(data);
            return val != 0;
        }
        case STP_TypeID_MATRIX_2D:
        {
            const auto val = std::any_cast<Matrix>(data);
            return std::ranges::all_of(val.getData(), [](const std::vector<Number>& vec) {
                return std::ranges::all_of(vec, [](const Number& n) { return n != 0; });
            });
        }
        case STP_TypeID_STRING:
        {
            const auto val = std::any_cast<std::string>(data);
            return not val.empty();
        }
        case STP_TypeID_FUNC:
        {
            output::error("parser"s, "Cannot convert Func to a logical type"s);
            programSafeExit(1);
        }
        case STP_TypeID_SYMBOL:
        {
            output::error("parser"s, "Cannot convert Symbol to a logical type"s);
            programSafeExit(1);
        }
        }

        // Should not reach this
        return false;
    }

    void STP_Scope::addVariable(const std::string& name, const STP_Value& data)
    {
        variables.insert_or_assign(name, data);
    }

    STP_Value STP_Scope::getVariable(const std::string& name)
    {
        if (not variables.contains(name))
        {
            if (parentScope == nullptr)
            {
                output::error("parser"s, "Variable {0} is not defined"s, { name });
                programSafeExit(1);
            }
            return parentScope->getVariable(name);
        }
        return variables.at(name);
    }

    void STP_Scope::addFunction(const std::string& name, const STP_FunctionDefinition& fn) { functions[name] = fn; }

    STP_FunctionDefinition STP_Scope::getFunction(const std::string& name)
    {
        if (functions.contains(name))
            return functions[name];
        output::error("runtime"s, "Cannot find function {0} in scope"s, { name });
        programSafeExit(1);
        return {};
    }

    STP_InterpStoreLocal::STP_InterpStoreLocal()
    {
        STP_DynamicLibrary stpFnLib("steppable");
        loadedLibraries.push_back(stpFnLib);
    }

    void STP_InterpStoreLocal::setChunk(const std::string& newChunk, const size_t& chunkStart, const size_t& chunkEnd)
    {
        chunk = newChunk;
        this->chunkStart = chunkStart;
        this->chunkEnd = chunkEnd;
    }

    std::string STP_InterpStoreLocal::getChunk(const TSNode* node)
    {
        if (node == nullptr)
            return chunk;

        uint32_t start = ts_node_start_byte(*node);
        uint32_t end = ts_node_end_byte(*node);
        std::string text;
        if (end - chunkStart <= chunk.size())
            text = chunk.substr(start - chunkStart, end - start);
        return text;
    }

    STP_Scope STP_InterpStoreLocal::addChildScope(std::shared_ptr<STP_Scope> parent) const
    {
        STP_Scope scope;
        if (not parent)
            scope.parentScope = currentScope;
        else
            scope.parentScope = std::move(parent);
        return scope;
    }

    void STP_InterpStoreLocal::setCurrentScope(std::shared_ptr<STP_Scope> newScope)
    {
        currentScope = std::move(newScope);
    }

    void STP_InterpStoreLocal::setFile(const std::string& newFile) { file = newFile; }

    std::string STP_InterpStoreLocal::getFile() const { return file; }
} // namespace steppable::parser
