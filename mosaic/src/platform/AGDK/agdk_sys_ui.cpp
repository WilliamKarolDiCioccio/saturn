#include "agdk_sys_ui.hpp"

namespace mosaic
{
namespace platform
{
namespace agdk
{

std::optional<bool> AGDKSystemUI::showQuestionDialog(const std::string& _title,
                                                     const std::string& _message,
                                                     bool _allowCancel) const
{
    auto* helper = JNIHelper::getInstance();

    JNIEnv* env = helper->getEnv();
    if (!env)
    {
        MOSAIC_ERROR("AGDKSystemUI::showQuestionDialog: JNI environment not available.");
        return std::nullopt;
    }

    jstring jTitle = helper->stringToJstring(_title);
    jstring jMessage = helper->stringToJstring(_message);
    jboolean jAllowCancel = static_cast<jboolean>(_allowCancel);

    // See SystemUI.showQuestionDialog(String title, String message, boolean allowCancel)
    jobject jResult = helper->callStaticMethod<jobject>(
        "com/mosaic/engine_bridge/SystemUI", "showQuestionDialog",
        "(Ljava/lang/String;Ljava/lang/String;Z)Ljava/lang/Boolean;", jTitle, jMessage,
        jAllowCancel);

    if (jTitle) env->DeleteLocalRef(jTitle);
    if (jMessage) env->DeleteLocalRef(jMessage);

    if (!jResult)
    {
        MOSAIC_ERROR("AGDKSystemUI::showQuestionDialog: Failed to call SystemUI method.");
        return std::nullopt;
    }

    jclass booleanClass = env->FindClass("java/lang/Boolean");
    jmethodID booleanValue = env->GetMethodID(booleanClass, "booleanValue", "()Z");

    return env->CallBooleanMethod(jResult, booleanValue);
}

void AGDKSystemUI::showInfoDialog(const std::string& _title, const std::string& _message) const
{
    auto* helper = JNIHelper::getInstance();

    JNIEnv* env = helper->getEnv();
    if (!env)
    {
        MOSAIC_ERROR("AGDKSystemUI::showInfoDialog: JNI environment not available.");
        return;
    }

    jstring jTitle = helper->stringToJstring(_title);
    jstring jMessage = helper->stringToJstring(_message);

    // See SystemUI.showInfoDialog(String title, String message)
    helper->callStaticVoidMethod("com/mosaic/engine_bridge/SystemUI", "showInfoDialog",
                                 "(Ljava/lang/String;Ljava/lang/String;)V", jTitle, jMessage);

    if (jTitle) env->DeleteLocalRef(jTitle);
    if (jMessage) env->DeleteLocalRef(jMessage);
}

void AGDKSystemUI::showWarningDialog(const std::string& _title, const std::string& _message) const
{
    auto* helper = JNIHelper::getInstance();

    JNIEnv* env = helper->getEnv();
    if (!env)
    {
        MOSAIC_ERROR("AGDKSystemUI::showWarningDialog: JNI environment not available.");
        return;
    }

    jstring jTitle = helper->stringToJstring(_title);
    jstring jMessage = helper->stringToJstring(_message);

    // See SystemUI.showWarningDialog(String title, String message)
    helper->callStaticVoidMethod("com/mosaic/engine_bridge/SystemUI", "showWarningDialog",
                                 "(Ljava/lang/String;Ljava/lang/String;)V", jTitle, jMessage);

    if (jTitle) env->DeleteLocalRef(jTitle);
    if (jMessage) env->DeleteLocalRef(jMessage);
}

void AGDKSystemUI::showErrorDialog(const std::string& _title, const std::string& _message) const
{
    auto* helper = JNIHelper::getInstance();

    JNIEnv* env = helper->getEnv();
    if (!env)
    {
        MOSAIC_ERROR("AGDKSystemUI::showErrorDialog: JNI environment not available.");
        return;
    }

    jstring jTitle = helper->stringToJstring(_title);
    jstring jMessage = helper->stringToJstring(_message);

    // See SystemUI.showErrorDialog(String title, String message)
    helper->callStaticVoidMethod("com/mosaic/engine_bridge/SystemUI", "showErrorDialog",
                                 "(Ljava/lang/String;Ljava/lang/String;)V", jTitle, jMessage);

    if (jTitle) env->DeleteLocalRef(jTitle);
    if (jMessage) env->DeleteLocalRef(jMessage);
}

} // namespace agdk
} // namespace platform
} // namespace mosaic