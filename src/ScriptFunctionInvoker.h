#ifndef CK_SCRIPTFUNCTIONINVOKER_H
#define CK_SCRIPTFUNCTIONINVOKER_H

#include <string>
#include <functional>
#include <type_traits>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <typeindex>

#include <angelscript.h>

class ScriptTypeRegistry {
public:
    using ArgHandler = std::function<void(asIScriptContext *, int, void *)>;
    using ReturnHandler = std::function<void(asIScriptContext *, void *)>;

    template<typename T>
    using TypedArgHandler = std::function<void(asIScriptContext *, int, T &)>;
    template<typename T>
    using TypedReturnHandler = std::function<void(asIScriptContext *, T &)>;

private:
    static inline std::unordered_map<std::type_index, ArgHandler> argHandlers;
    static inline std::unordered_map<std::type_index, ReturnHandler> returnHandlers;

public:
    // Register argument handler
    template<typename T>
    static void RegisterArgHandler(const TypedArgHandler<T> &handler) {
        argHandlers.emplace(std::type_index(typeid(T)), [handler](asIScriptContext *ctx, int index, void *arg) {
            handler(ctx, index, *static_cast<T *>(arg));
        });
    }

    // Register return handler
    template<typename T>
    static void RegisterReturnHandler(const TypedReturnHandler<T> &handler) {
        returnHandlers.emplace(std::type_index(typeid(T)), [handler](asIScriptContext *ctx, void *ret) {
            handler(ctx, *static_cast<T *>(ret));
        });
    }

    // Set argument using the registry
    template<typename T>
    static void SetArg(asIScriptContext *ctx, int index, T &value) {
        auto it = argHandlers.find(std::type_index(typeid(T)));
        if (it != argHandlers.end()) {
            it->second(ctx, index, &value);
        } else {
            // Fallback to default handling
            DefaultSetArg(ctx, index, value);
        }
    }

    // Get return value using the registry
    template<typename T>
    static void GetReturn(asIScriptContext *ctx, T &value) {
        auto it = returnHandlers.find(std::type_index(typeid(T)));
        if (it != returnHandlers.end()) {
            it->second(ctx, &value);
        } else {
            // Fallback to default handling
            DefaultGetReturn(ctx, value);
        }
    }

private:
    template<typename T>
    static void HandleFallback(asIScriptContext *ctx, int index, T &value, bool isReturn = false) {
        if constexpr (std::is_integral_v<T>) {
            if (isReturn) {
                value = static_cast<T>(ctx->GetReturnDWord());
            } else {
                ctx->SetArgDWord(index, static_cast<asDWORD>(value));
            }
        } else if constexpr (std::is_same_v<std::remove_cv<T>, float>) {
            if (isReturn) {
                value = ctx->GetReturnFloat();
            } else {
                ctx->SetArgFloat(index, value);
            }
        } else if constexpr (std::is_same_v<std::remove_cv<T>, double>) {
            if (isReturn) {
                value = ctx->GetReturnDouble();
            } else {
                ctx->SetArgDouble(index, value);
            }
        } else if constexpr (std::is_pointer_v<T>) {
            if (isReturn) {
                value = reinterpret_cast<T>(ctx->GetReturnObject());
            } else {
                ctx->SetArgObject(index, value);
            }
        } else {
            throw std::invalid_argument("Unsupported argument or return type");
        }
    }

    template<typename T>
    static void DefaultSetArg(asIScriptContext *ctx, int index, T &value) {
        HandleFallback(ctx, index, value, false);
    }

    template<typename T>
    static void DefaultGetReturn(asIScriptContext *ctx, T &value) {
        HandleFallback(ctx, 0, value, true);
    }
};

template<typename First, typename... Rest>
static constexpr decltype(auto) GetLastArgument(First &&first, Rest &&... rest) {
    if constexpr (sizeof...(Rest) == 0) {
        // Base case: `First` is the last argument.
        return std::forward<First>(first);
    } else {
        // Recursively process the remaining arguments.
        return GetLastArgument(std::forward<Rest>(rest)...);
    }
}

template<typename Ret, typename... Args>
struct ScriptFunctionInvoker {};

template<typename Ret, typename... Args>
struct ScriptFunctionInvoker<Ret(*)(Args...)> {
    static Ret Invoke(Args... args) {
        void *data = GetLastArgument(args...);
        auto *cb = static_cast<asIScriptFunction *>(data);
        if (!cb) {
            throw std::runtime_error("Invalid script function pointer");
        }

        asIScriptEngine *engine = cb->GetEngine();
        asIScriptContext *ctx = engine->RequestContext();
        if (!ctx)
            throw std::runtime_error("Failed to create AngelScript context");

        if (cb->GetFuncType() == asFUNC_DELEGATE) {
            asIScriptFunction *callback = cb->GetDelegateFunction();
            void *callbackObject = cb->GetDelegateObject();
            ctx->Prepare(callback);
            ctx->SetObject(callbackObject);
        } else {
            ctx->Prepare(cb);
        }

        // Set arguments using ScriptTypeRegistry
        int index = 0;
        (..., ScriptTypeRegistry::SetArg(ctx, index++, args));

        int r = ctx->Execute();
        engine->ReturnContext(ctx);

        if (r == asEXECUTION_EXCEPTION) {
            const char *exception = ctx->GetExceptionString();
            std::string funcName = cb->GetDeclaration();
            throw std::runtime_error("Script execution exception in function '" +
                                     funcName + "': " +
                                     (exception ? exception : "Unknown exception"));
        } else if (r != asEXECUTION_FINISHED) {
            throw std::runtime_error("Script execution failed with result code: " + std::to_string(r));
        }

        // Use ScriptTypeRegistry for return value handling
        if constexpr (std::is_same_v<Ret, void>) {
            return; // For void return type
        } else {
            Ret result;
            ScriptTypeRegistry::GetReturn(ctx, result);
            return result;
        }
    }
};



#endif // CK_SCRIPTFUNCTIONINVOKER_H
