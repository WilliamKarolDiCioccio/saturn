#pragma once

#include <filesystem>
#include <vector>

#include "../models/mdx_file.hpp"

namespace docsgen
{

class FileWriter
{
   public:
    static bool write(const std::vector<MDXFile>& files, const std::filesystem::path& outputDir,
                      bool verbose = false);

   private:
    static std::string buildFullMDX(const MDXFile& file);
};

} // namespace docsgen
