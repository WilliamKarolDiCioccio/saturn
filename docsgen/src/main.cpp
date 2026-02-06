#include <filesystem>
#include <iostream>
#include <vector>

#include <codex/analyzer.hpp>
#include <codex/files_collector.hpp>
#include <codex/parser.hpp>
#include <codex/source_extractor.hpp>

#include "cli/cli_parser.hpp"
#include "generators/mdx_generator.hpp"
#include "writers/file_writer.hpp"

int main(int argc, char* argv[])
{
    // Parse CLI
    auto cliResult = docsgen::parseCLI(argc, argv);
    if (!cliResult.success)
    {
        std::cerr << "Error: " << cliResult.errorMessage << "\n";
        std::cerr << "Usage: docsgen --input <dir> --output <dir> [--verbose]\n";
        return 1;
    }

    if (cliResult.options.showHelp)
    {
        std::cout << "docsgen - Generate MDX documentation from C++ headers\n\n";
        std::cout << "Usage: docsgen --input <dir> --output <dir> [--verbose]\n\n";
        std::cout << "Options:\n";
        std::cout << "  -i, --input <path>   Input directory with C++ headers (required)\n";
        std::cout << "  -o, --output <path>  Output directory for MDX files (required)\n";
        std::cout << "  -v, --verbose        Enable verbose output\n";
        std::cout << "  -h, --help           Show this help message\n";
        return 0;
    }

    const auto& opts = cliResult.options;

    if (opts.verbose)
    {
        std::cout << "Input:  " << opts.inputDir << "\n";
        std::cout << "Output: " << opts.outputDir << "\n";
    }

    // Step 1: Collect files
    if (opts.verbose) std::cout << "\nCollecting files...\n";

    codex::FilesCollector collector({".hpp", ".h"}, {opts.inputDir.string()}, true);
    std::vector<std::filesystem::path> files = collector.collect();

    if (opts.verbose) std::cout << "  Found " << files.size() << " header files\n";

    if (files.empty())
    {
        std::cerr << "No header files found in: " << opts.inputDir << "\n";
        return 1;
    }

    // Step 2: Extract sources
    if (opts.verbose) std::cout << "\nExtracting sources...\n";

    codex::SourceExtractor extractor;
    std::vector<std::shared_ptr<codex::Source>> sources;
    sources.reserve(files.size());

    for (const auto& file : files)
    {
        auto source = extractor.extractSource(file);
        if (source) sources.push_back(source);
    }

    if (opts.verbose) std::cout << "  Extracted " << sources.size() << " sources\n";

    // Step 3: Parse
    if (opts.verbose) std::cout << "\nParsing...\n";

    codex::Parser parser;
    std::vector<std::shared_ptr<codex::SourceNode>> sourceNodes;
    sourceNodes.reserve(sources.size());

    for (const auto& source : sources)
    {
        auto node = parser.parse(source);
        if (node) sourceNodes.push_back(node);
        parser.reset();
    }

    if (opts.verbose) std::cout << "  Parsed " << sourceNodes.size() << " files\n";

    // Step 4: Analyze
    if (opts.verbose) std::cout << "\nAnalyzing...\n";

    codex::Analyzer analyzer;
    codex::AnalysisResult result = analyzer.analyze(sourceNodes);

    if (opts.verbose) std::cout << "  Indexed " << result.database.size() << " symbols\n";

    // Step 5: Generate MDX
    if (opts.verbose) std::cout << "\nGenerating MDX...\n";

    docsgen::MDXGenerator generator;
    generator.setVerbose(opts.verbose);
    std::vector<docsgen::MDXFile> mdxFiles = generator.generate(result);

    if (opts.verbose) std::cout << "  Generated " << mdxFiles.size() << " MDX files\n";

    // Step 6: Write files
    if (opts.verbose) std::cout << "\nWriting files...\n";

    bool success = docsgen::FileWriter::write(mdxFiles, opts.outputDir, opts.verbose);

    if (success)
    {
        std::cout << "Done! Generated " << mdxFiles.size() << " MDX files in " << opts.outputDir
                  << "\n";
        return 0;
    }
    else
    {
        std::cerr << "Some files failed to write\n";
        return 1;
    }
}
