#include <gtest/gtest.h>

#include <saturn/tools/logger.hpp>
#include <saturn/core/cmd_line_parser.hpp>

int main(int _argc, char** _argv)
{
    saturn::core::CommandLineParser::initialize();

    saturn::tools::Logger::initialize();

    ::testing::InitGoogleTest(&_argc, _argv);
    int result = RUN_ALL_TESTS();

    saturn::tools::Logger::shutdown();

    saturn::core::CommandLineParser::shutdown();

    return result;
}
