#include <windows.h>

#include <gtest/gtest.h>

#include <mosaic/core/logger.hpp>

bool initializeWin32Platform()
{
    HRESULT hres = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (FAILED(hres)) return false;

    hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT,
                                RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
    if (FAILED(hres))
    {
        CoUninitialize();
        return false;
    }

    return true;
}

void shutdownWin32Platform() { CoUninitialize(); }

int main(int _argc, char **_argv)
{
    using namespace mosaic::core;

#ifdef MOSAIC_PLATFORM_WINDOWS
    if (!initializeWin32Platform()) return -1;
#endif

    LoggerManager::initialize();

    ::testing::InitGoogleTest(&_argc, _argv);
    int result = RUN_ALL_TESTS();

    LoggerManager::shutdown();

    return result;
}
