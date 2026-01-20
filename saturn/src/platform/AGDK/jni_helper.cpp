#include "saturn/platform/AGDK/jni_helper.hpp"

namespace saturn
{
namespace platform
{
namespace agdk
{

std::unique_ptr<JNIHelper> JNIHelper::m_instance = std::make_unique<JNIHelper>();

thread_local JNIEnv* JNIHelper::m_tlsEnv = nullptr;

pieces::RefResult<JNIHelper, std::string> JNIHelper::initialize(JavaVM* _vm)
{
    if (m_initialized.load())
    {
        SATURN_WARN("JNIHelper::initialize(): already initialized");
        return pieces::OkRef<JNIHelper, std::string>(*this);
    }

    if (!_vm)
    {
        SATURN_ERROR("JNIHelper::initialize(): JavaVM is null");
        return pieces::ErrRef<JNIHelper, std::string>("JavaVM is null");
    }

    m_javaVM = _vm;
    m_initialized.store(true);

    SATURN_INFO("JNIHelper initialized successfully");

    return pieces::OkRef<JNIHelper, std::string>(*this);
}

void JNIHelper::shutdown()
{
    if (!m_initialized.load()) return;

    JNIEnv* env = getEnv();

    if (env)
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);

        for (auto& pair : m_classCache)
        {
            if (pair.second)
            {
                env->DeleteGlobalRef(pair.second);
            }
        }

        m_classCache.clear();
        m_methodCache.clear();
        m_fieldCache.clear();
    }

    m_initialized.store(false);

    SATURN_INFO("JNIHelper shutdown complete");
}

JNIEnv* JNIHelper::getEnv()
{
    if (!m_initialized.load())
    {
        SATURN_ERROR("JNIHelper not initialized");
        return nullptr;
    }

    // Check thread-local storage first
    if (m_tlsEnv) return m_tlsEnv;

    // Try to get JNIEnv for current thread
    JNIEnv* env = nullptr;
    jint result = m_javaVM->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);

    if (result == JNI_OK)
    {
        m_tlsEnv = env;
        return env;
    }
    else if (result == JNI_EDETACHED)
    {
        // Thread not attached, attach it
        return attachCurrentThread();
    }
    else
    {
        SATURN_ERROR("Failed to get JNIEnv, error: %d", result);
        return nullptr;
    }
}

void JNIHelper::releaseEnv()
{
    // For proper cleanup when thread ends
    m_tlsEnv = nullptr;
}

JNIEnv* JNIHelper::attachCurrentThread()
{
    JNIEnv* env = nullptr;
    JavaVMAttachArgs args = {JNI_VERSION_1_6, "GameEngineThread", nullptr};

    jint result = m_javaVM->AttachCurrentThread(&env, &args);
    if (result == JNI_OK)
    {
        m_tlsEnv = env;
        SATURN_INFO("Thread attached successfully");
        return env;
    }
    else
    {
        SATURN_ERROR("Failed to attach thread, error: %d", result);
        return nullptr;
    }
}

void JNIHelper::detachCurrentThread()
{
    if (m_tlsEnv)
    {
        m_javaVM->DetachCurrentThread();
        m_tlsEnv = nullptr;
        SATURN_INFO("Thread detached");
    }
}

jclass JNIHelper::findClass(const std::string& _className)
{
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);

        auto it = m_classCache.find(_className);
        if (it != m_classCache.end()) return it->second;
    }

    JNIEnv* env = getEnv();
    if (!env) return nullptr;

    jclass localClass = env->FindClass(_className.c_str());
    if (!localClass)
    {
        SATURN_ERROR("Failed to find class: %s", _className.c_str());
        checkAndClearException();
        return nullptr;
    }

    // Create global reference
    jclass globalClass = static_cast<jclass>(env->NewGlobalRef(localClass));

    env->DeleteLocalRef(localClass);

    if (globalClass)
    {
        m_classCache[_className] = globalClass;
        SATURN_INFO("Cached class: %s", _className.c_str());
    }

    return globalClass;
}

jmethodID JNIHelper::getMethodID(const std::string& _className, const std::string& _methodName,
                                 const std::string& _signature)
{
    std::string key = createCacheKey(_className, _methodName, _signature);

    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);

        auto it = m_methodCache.find(key);
        if (it != m_methodCache.end()) return it->second;
    }

    jclass clazz = findClass(_className);
    if (!clazz) return nullptr;

    JNIEnv* env = getEnv();
    if (!env) return nullptr;

    jmethodID method = env->GetMethodID(clazz, _methodName.c_str(), _signature.c_str());
    if (!method)
    {
        SATURN_ERROR("Failed to get method ID: %s.%s%s", _className.c_str(), _methodName.c_str(),
                     _signature.c_str());
        checkAndClearException();
        return nullptr;
    }

    m_methodCache[key] = method;

    return method;
}

jmethodID JNIHelper::getStaticMethodID(const std::string& _className,
                                       const std::string& _methodName,
                                       const std::string& _signature)
{
    std::string key = createCacheKey(_className, "static_" + _methodName, _signature);

    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);

        auto it = m_methodCache.find(key);
        if (it != m_methodCache.end()) return it->second;
    }

    jclass clazz = findClass(_className);
    if (!clazz) return nullptr;

    JNIEnv* env = getEnv();
    if (!env) return nullptr;

    jmethodID method = env->GetStaticMethodID(clazz, _methodName.c_str(), _signature.c_str());
    if (!method)
    {
        SATURN_ERROR("Failed to get static method ID: %s.%s%s", _className.c_str(),
                     _methodName.c_str(), _signature.c_str());
        checkAndClearException();
        return nullptr;
    }

    m_methodCache[key] = method;

    return method;
}

jfieldID JNIHelper::getFieldID(const std::string& _className, const std::string& _fieldName,
                               const std::string& _signature)
{
    std::string key = createCacheKey(_className, _fieldName, _signature);

    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);

        auto it = m_fieldCache.find(key);
        if (it != m_fieldCache.end()) return it->second;
    }

    jclass clazz = findClass(_className);
    if (!clazz) return nullptr;

    JNIEnv* env = getEnv();
    if (!env) return nullptr;

    jfieldID field = env->GetFieldID(clazz, _fieldName.c_str(), _signature.c_str());
    if (!field)
    {
        SATURN_ERROR("Failed to get field ID: %s.%s", _className.c_str(), _fieldName.c_str());
        checkAndClearException();
        return nullptr;
    }

    m_fieldCache[key] = field;

    return field;
}

jfieldID JNIHelper::getStaticFieldID(const std::string& _className, const std::string& _fieldName,
                                     const std::string& _signature)
{
    std::string key = createCacheKey(_className, "static_" + _fieldName, _signature);

    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);

        auto it = m_fieldCache.find(key);
        if (it != m_fieldCache.end()) return it->second;
    }

    jclass clazz = findClass(_className);
    if (!clazz) return nullptr;

    JNIEnv* env = getEnv();
    if (!env) return nullptr;

    jfieldID field = env->GetStaticFieldID(clazz, _fieldName.c_str(), _signature.c_str());
    if (!field)
    {
        SATURN_ERROR("Failed to get static field ID: %s.%s", _className.c_str(),
                     _fieldName.c_str());
        checkAndClearException();
        return nullptr;
    }

    m_fieldCache[key] = field;

    return field;
}

bool JNIHelper::registerNativeMethods(const std::string& _className,
                                      const std::vector<NativeMethod>& _methods)
{
    jclass clazz = findClass(_className);
    if (!clazz) return false;

    JNIEnv* env = getEnv();
    if (!env) return false;

    std::vector<JNINativeMethod> jniMethods;
    jniMethods.reserve(_methods.size());

    for (const auto& method : _methods)
    {
        JNINativeMethod jniMethod;
        jniMethod.name = method.name.c_str();
        jniMethod.signature = method.signature.c_str();
        jniMethod.fnPtr = method.fnPtr;
        jniMethods.push_back(jniMethod);
    }

    jint result =
        env->RegisterNatives(clazz, jniMethods.data(), static_cast<jint>(jniMethods.size()));
    if (result != JNI_OK)
    {
        SATURN_ERROR("Failed to register native methods for class: %s", _className.c_str());
        checkAndClearException();
        return false;
    }

    SATURN_INFO("Registered %zu native methods for class: %s", _methods.size(), _className.c_str());

    return true;
}

std::string JNIHelper::jstringToString(jstring _jstr)
{
    if (!_jstr) return "";

    JNIEnv* env = getEnv();
    if (!env) return "";

    const char* chars = env->GetStringUTFChars(_jstr, nullptr);
    if (!chars) return "";

    std::string result(chars);
    env->ReleaseStringUTFChars(_jstr, chars);
    return result;
}

jstring JNIHelper::stringToJstring(const std::string& _str)
{
    JNIEnv* env = getEnv();
    if (!env) return nullptr;

    return env->NewStringUTF(_str.c_str());
}

std::vector<std::string> JNIHelper::jstringArrayToVector(jobjectArray _jarray)
{
    std::vector<std::string> result;
    if (!_jarray) return result;

    JNIEnv* env = getEnv();
    if (!env) return result;

    jsize length = env->GetArrayLength(_jarray);
    result.reserve(length);

    for (jsize i = 0; i < length; ++i)
    {
        jstring jstr = static_cast<jstring>(env->GetObjectArrayElement(_jarray, i));
        if (jstr)
        {
            result.push_back(jstringToString(jstr));
            env->DeleteLocalRef(jstr);
        }
    }

    return result;
}

jobjectArray JNIHelper::vectorToJstringArray(const std::vector<std::string>& _vec)
{
    JNIEnv* env = getEnv();
    if (!env) return nullptr;

    jclass stringClass = findClass("java/lang/String");
    if (!stringClass) return nullptr;

    jobjectArray jarray =
        env->NewObjectArray(static_cast<jsize>(_vec.size()), stringClass, nullptr);
    if (!jarray) return nullptr;

    for (size_t i = 0; i < _vec.size(); ++i)
    {
        jstring jstr = stringToJstring(_vec[i]);
        if (jstr)
        {
            env->SetObjectArrayElement(jarray, static_cast<jsize>(i), jstr);
            env->DeleteLocalRef(jstr);
        }
    }

    return jarray;
}

jobject JNIHelper::createGlobalRef(jobject _obj)
{
    if (!_obj) return nullptr;

    JNIEnv* env = getEnv();
    if (!env) return nullptr;

    return env->NewGlobalRef(_obj);
}

void JNIHelper::deleteGlobalRef(jobject _globalRef)
{
    if (!_globalRef) return;

    JNIEnv* env = getEnv();
    if (!env) return;

    env->DeleteGlobalRef(_globalRef);
}

bool JNIHelper::checkAndClearException()
{
    JNIEnv* env = getEnv();
    if (!env) return false;

    if (env->ExceptionCheck())
    {
        env->ExceptionDescribe(); // Print to logcat
        env->ExceptionClear();
        return true;
    }
    return false;
}

void JNIHelper::throwJavaException(const std::string& _exceptionClass, const std::string& _message)
{
    JNIEnv* env = getEnv();
    if (!env) return;

    jclass clazz = findClass(_exceptionClass);
    if (clazz)
    {
        env->ThrowNew(clazz, _message.c_str());
    }
}

void JNIHelper::registerCallback(const std::string& _callbackName, CallbackFunction _callback)
{
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_callbacks[_callbackName] = std::move(_callback);
    SATURN_INFO("Registered callback: %s", _callbackName.c_str());
}

void JNIHelper::invokeCallback(const std::string& _callbackName,
                               const std::vector<std::string>& _args)
{
    std::lock_guard<std::mutex> lock(m_callbackMutex);

    auto it = m_callbacks.find(_callbackName);
    if (it != m_callbacks.end())
    {
        it->second(_args);
    }
    else
    {
        SATURN_ERROR("Callback not found: %s", _callbackName.c_str());
    }
}

std::string JNIHelper::createCacheKey(const std::string& _className, const std::string& _memberName,
                                      const std::string& _signature)
{
    return _className + "::" + _memberName + "::" + _signature;
}

// ThreadAttachment implementation
JNIHelper::ThreadAttachment::ThreadAttachment() : m_env(nullptr), m_shouldDetach(false)
{
    auto helper = JNIHelper::getInstance();

    if (!helper->m_initialized.load()) return;

    // Check if thread is already attached
    jint result = helper->m_javaVM->GetEnv(reinterpret_cast<void**>(&m_env), JNI_VERSION_1_6);
    if (result == JNI_EDETACHED)
    {
        // Need to attach
        m_env = helper->attachCurrentThread();
        m_shouldDetach = (m_env != nullptr);
    }
}

JNIHelper::ThreadAttachment::~ThreadAttachment()
{
    if (m_shouldDetach && m_env) JNIHelper::getInstance()->detachCurrentThread();
}

} // namespace agdk
} // namespace platform
} // namespace saturn
