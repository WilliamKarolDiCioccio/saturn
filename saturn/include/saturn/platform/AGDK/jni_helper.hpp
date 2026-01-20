#pragma once

#include <jni.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>

#include <pieces/core/result.hpp>

#include "saturn/tools/logger.hpp"

namespace saturn
{
namespace platform
{
namespace agdk
{

/**
 * @brief The `JNIHelper` class provides a singleton interface for managing JNI interactions.
 *
 * The `JNIHelper` class enables:
 *
 * - Class, fields and method lookups.
 *
 * - Method invocation with type safety (both static and instance methods).
 *
 * - String and array utilities for converting between Java and C++ types.
 *
 * - Exception handling and conversion between Java and C++ exceptions.
 *
 * - Dynamic native method callbacks registration.
 *
 * - Thread safe access to JNI environment via thread-local storage and RAII-style thread
 * attachment.
 *
 * @note Always keep in mind you need the correct class loader context when performing lookups.
 * @note Java version dependent references all point to Java 11 documentation as it's the target
 * version of the Android project.
 *
 * @see https://docs.oracle.com/en/java/javase/11/docs/api/java.base/java/lang/ClassLoader.html
 * @see https://docs.oracle.com/en/java/javase/11/docs/specs/jni/index.html
 * @see https://developer.android.com/training/articles/perf-jni#native-libraries
 */
class JNIHelper
{
   private:
    using CallbackFunction = std::function<void(const std::vector<std::string>&)>;

    static std::unique_ptr<JNIHelper> m_instance;

    JavaVM* m_javaVM;
    std::atomic<bool> m_initialized;

    // Caches
    std::unordered_map<std::string, jclass> m_classCache;
    std::unordered_map<std::string, jmethodID> m_methodCache;
    std::unordered_map<std::string, jfieldID> m_fieldCache;

    // Thread-local storage for JNIEnv*
    thread_local static JNIEnv* m_tlsEnv;

    // Callback system
    std::unordered_map<std::string, CallbackFunction> m_callbacks;
    std::mutex m_callbackMutex;

    mutable std::mutex m_cacheMutex;

   public:
    JNIHelper() = default;
    ~JNIHelper() = default;

    JNIHelper(JNIHelper&) = delete;
    JNIHelper& operator=(JNIHelper&) = delete;
    JNIHelper(JNIHelper&&) = delete;
    JNIHelper& operator=(JNIHelper&&) = delete;

   public:
    pieces::RefResult<JNIHelper, std::string> initialize(JavaVM* _vm);
    void shutdown();

    [[nodiscard]] static JNIHelper* getInstance() { return m_instance.get(); }

    // Environment management

    [[nodiscard]] JNIEnv* getEnv();
    void releaseEnv();

    // Class and method management

    jclass findClass(const std::string& _className);
    jmethodID getMethodID(const std::string& _className, const std::string& _methodName,
                          const std::string& _signature);
    jmethodID getStaticMethodID(const std::string& _className, const std::string& _methodName,
                                const std::string& _signature);
    jfieldID getFieldID(const std::string& _className, const std::string& _fieldName,
                        const std::string& _signature);
    jfieldID getStaticFieldID(const std::string& _className, const std::string& _fieldName,
                              const std::string& _signature);

    // Method calling wrappers

    template <typename... Args>
    void callVoidMethod(jobject _obj, const std::string& _className, const std::string& _methodName,
                        const std::string& _signature, Args... _args);
    template <typename T, typename... Args>
    T callMethod(jobject _obj, const std::string& _className, const std::string& _methodName,
                 const std::string& signature, Args... _args);
    template <typename... Args>
    void callStaticVoidMethod(const std::string& _className, const std::string& _methodName,
                              const std::string& _signature, Args... _args);
    template <typename T, typename... Args>
    T callStaticMethod(const std::string& _className, const std::string& _methodName,
                       const std::string& _signature, Args... _args);

    // String utilities

    std::string jstringToString(jstring _jstr);
    jstring stringToJstring(const std::string& _str);

    // Array utilities

    std::vector<std::string> jstringArrayToVector(jobjectArray _jarray);
    jobjectArray vectorToJstringArray(const std::vector<std::string>& _vec);

    // Global reference management

    jobject createGlobalRef(jobject _obj);
    void deleteGlobalRef(jobject _globalRef);

    // Exception handling

    bool checkAndClearException();
    void throwJavaException(const std::string& _exceptionClass, const std::string& _message);

    // Dynamic native method binding

    struct NativeMethod
    {
        std::string name;
        std::string signature;
        void* fnPtr;
    };

    bool registerNativeMethods(const std::string& _className,
                               const std::vector<NativeMethod>& _methods);

    /**
     * @brief ThreadAttachment is a RAII class that automatically attaches the current thread to the
     * JNI environment.
     */
    class ThreadAttachment
    {
       public:
        ThreadAttachment();
        ~ThreadAttachment();

        JNIEnv* getEnv() const { return m_env; }
        bool isAttached() const { return m_env != nullptr; }

       private:
        JNIEnv* m_env;
        bool m_shouldDetach;
    };

    // Callback system for Kotlin -> C++ communication
    void registerCallback(const std::string& _callbackName, CallbackFunction _callback);
    void invokeCallback(const std::string& _callbackName, const std::vector<std::string>& _args);

   private:
    std::string createCacheKey(const std::string& _className, const std::string& _memberName,
                               const std::string& _signature = "");

    JNIEnv* attachCurrentThread();
    void detachCurrentThread();
};

template <typename... Args>
void JNIHelper::callVoidMethod(jobject _obj, const std::string& _className,
                               const std::string& _methodName, const std::string& _signature,
                               Args... _args)
{
    JNIEnv* env = getEnv();
    if (!env) return;

    jmethodID method = getMethodID(_className, _methodName, _signature);
    if (method)
    {
        env->CallVoidMethod(_obj, method, _args...);
        checkAndClearException();
    }
}

template <typename T, typename... Args>
T JNIHelper::callMethod(jobject _obj, const std::string& _className, const std::string& _methodName,
                        const std::string& _signature, Args... _args)
{
    JNIEnv* env = getEnv();
    if (!env) return T{};

    jmethodID method = getMethodID(_className, _methodName, _signature);
    if (!method) return T{};

    T result;
    if constexpr (std::is_same_v<T, jint>)
    {
        result = env->CallIntMethod(_obj, method, _args...);
    }
    else if constexpr (std::is_same_v<T, jlong>)
    {
        result = env->CallLongMethod(_obj, method, _args...);
    }
    else if constexpr (std::is_same_v<T, jfloat>)
    {
        result = env->CallFloatMethod(_obj, method, _args...);
    }
    else if constexpr (std::is_same_v<T, jdouble>)
    {
        result = env->CallDoubleMethod(_obj, method, _args...);
    }
    else if constexpr (std::is_same_v<T, jboolean>)
    {
        result = env->CallBooleanMethod(_obj, method, _args...);
    }
    else if constexpr (std::is_same_v<T, jobject>)
    {
        result = env->CallObjectMethod(_obj, method, _args...);
    }

    checkAndClearException();
    return result;
}

template <typename... Args>
void JNIHelper::callStaticVoidMethod(const std::string& _className, const std::string& _methodName,
                                     const std::string& _signature, Args... _args)
{
    JNIEnv* env = getEnv();
    if (!env) return;

    jclass clazz = findClass(_className);
    jmethodID method = getStaticMethodID(_className, _methodName, _signature);
    if (clazz && method)
    {
        env->CallStaticVoidMethod(clazz, method, _args...);
        checkAndClearException();
    }
}

template <typename T, typename... Args>
T JNIHelper::callStaticMethod(const std::string& _className, const std::string& _methodName,
                              const std::string& _signature, Args... _args)
{
    JNIEnv* env = getEnv();
    if (!env) return T{};

    jclass clazz = findClass(_className);
    jmethodID method = getStaticMethodID(_className, _methodName, _signature);
    if (!clazz || !method) return T{};

    T result;
    if constexpr (std::is_same_v<T, jint>)
    {
        result = env->CallStaticIntMethod(clazz, method, _args...);
    }
    else if constexpr (std::is_same_v<T, jlong>)
    {
        result = env->CallStaticLongMethod(clazz, method, _args...);
    }
    else if constexpr (std::is_same_v<T, jfloat>)
    {
        result = env->CallStaticFloatMethod(clazz, method, _args...);
    }
    else if constexpr (std::is_same_v<T, jdouble>)
    {
        result = env->CallStaticDoubleMethod(clazz, method, _args...);
    }
    else if constexpr (std::is_same_v<T, jboolean>)
    {
        result = env->CallStaticBooleanMethod(clazz, method, _args...);
    }
    else if constexpr (std::is_same_v<T, jobject>)
    {
        result = env->CallStaticObjectMethod(clazz, method, _args...);
    }

    checkAndClearException();
    return result;
}

#define JNI_METHOD_SIGNATURE(...) "(" #__VA_ARGS__ ")"
#define JNI_VOID_RETURN "V"
#define JNI_INT_RETURN "I"
#define JNI_LONG_RETURN "J"
#define JNI_FLOAT_RETURN "F"
#define JNI_DOUBLE_RETURN "D"
#define JNI_BOOLEAN_RETURN "Z"
#define JNI_STRING_RETURN "Ljava/lang/String;"
#define JNI_OBJECT_RETURN(className) "L" className ";"

} // namespace agdk
} // namespace platform
} // namespace saturn
