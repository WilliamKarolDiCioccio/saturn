#include <windows.h>

#include <gtest/gtest.h>

#include <mosaic/tools/logger.hpp>
#include <mosaic/core/cmd_line_parser.hpp>

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

int main(int _argc, char** _argv)
{
#ifdef MOSAIC_PLATFORM_WINDOWS
    if (!initializeWin32Platform()) return -1;
#endif

    mosaic::core::CommandLineParser::initialize();

    mosaic::tools::Logger::initialize();

    ::testing::InitGoogleTest(&_argc, _argv);
    int result = RUN_ALL_TESTS();

    mosaic::tools::Logger::shutdown();

    mosaic::core::CommandLineParser::shutdown();

    return result;
}
