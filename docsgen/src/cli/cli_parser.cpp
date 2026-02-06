#include "cli_parser.hpp"

#include <lyra/lyra.hpp>

namespace docsgen
{

CLIParseResult parseCLI(int argc, char* argv[])
{
    CLIParseResult result;
    std::string inputPath;
    std::string outputPath;

    auto cli =
        lyra::cli() |
        lyra::help(result.options.showHelp)
            .description("Generate MDX documentation from C++ headers") |
        lyra::opt(inputPath, "path")["-i"]["--input"]("Input directory with C++ headers (required)")
            .required() |
        lyra::opt(outputPath, "path")["-o"]["--output"]("Output directory for MDX files (required)")
            .required() |
        lyra::opt(result.options.verbose)["-v"]["--verbose"]("Enable verbose output");

    auto parseResult = cli.parse({argc, argv});

    if (!parseResult)
    {
        result.success = false;
        result.errorMessage = parseResult.message();
        return result;
    }

    if (result.options.showHelp)
    {
        result.success = true;
        return result;
    }

    result.options.inputDir = inputPath;
    result.options.outputDir = outputPath;

    if (!std::filesystem::exists(result.options.inputDir))
    {
        result.success = false;
        result.errorMessage = "Input directory does not exist: " + inputPath;
        return result;
    }

    if (!std::filesystem::is_directory(result.options.inputDir))
    {
        result.success = false;
        result.errorMessage = "Input path is not a directory: " + inputPath;
        return result;
    }

    result.success = true;
    return result;
}

} // namespace docsgen
