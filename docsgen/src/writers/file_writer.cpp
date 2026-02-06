#include "file_writer.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

namespace docsgen
{

std::string FileWriter::buildFullMDX(const MDXFile& file)
{
    std::ostringstream oss;

    // YAML frontmatter
    oss << "---\n";
    oss << "title: \"" << file.title << "\"\n";
    if (!file.description.empty()) oss << "description: \"" << file.description << "\"\n";
    oss << "---\n\n";

    // Content
    oss << file.content;

    return oss.str();
}

bool FileWriter::write(const std::vector<MDXFile>& files, const std::filesystem::path& outputDir,
                       bool verbose)
{
    // Create output directory if needed
    if (!std::filesystem::exists(outputDir))
    {
        std::error_code ec;
        if (!std::filesystem::create_directories(outputDir, ec))
        {
            std::cerr << "Failed to create output directory: " << outputDir << "\n";
            return false;
        }
    }

    size_t written = 0;
    size_t failed = 0;

    for (const auto& file : files)
    {
        std::filesystem::path outPath = outputDir / (file.filename + ".mdx");
        std::string content = buildFullMDX(file);

        std::ofstream ofs(outPath, std::ios::binary);
        if (!ofs)
        {
            std::cerr << "Failed to open for writing: " << outPath << "\n";
            failed++;
            continue;
        }

        ofs.write(content.data(), static_cast<std::streamsize>(content.size()));
        ofs.close();

        if (verbose) std::cout << "  Wrote: " << outPath.filename() << "\n";

        written++;
    }

    if (verbose)
    {
        std::cout << "Written " << written << " files";
        if (failed > 0) std::cout << " (" << failed << " failed)";
        std::cout << "\n";
    }

    return failed == 0;
}

} // namespace docsgen
