#pragma once

#include <filesystem>
#include <string>

namespace docsgen
{

struct CLIOptions
{
    std::filesystem::path inputDir;
    std::filesystem::path outputDir;
    bool verbose = false;
    bool showHelp = false;
};

struct CLIParseResult
{
    CLIOptions options;
    bool success = false;
    std::string errorMessage;
};

CLIParseResult parseCLI(int argc, char* argv[]);

} // namespace docsgen
