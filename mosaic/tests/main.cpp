#include <gtest/gtest.h>

#include <mosaic/tools/logger.hpp>
#include <mosaic/core/cmd_line_parser.hpp>

int main(int _argc, char** _argv)
{
    mosaic::core::CommandLineParser::initialize();

    mosaic::tools::Logger::initialize();

    ::testing::InitGoogleTest(&_argc, _argv);
    int result = RUN_ALL_TESTS();

    mosaic::tools::Logger::shutdown();

    mosaic::core::CommandLineParser::shutdown();

    return result;
}
