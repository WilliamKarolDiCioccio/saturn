#include "codex/source_extractor.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <future>

namespace codex
{

std::shared_ptr<Source> SourceExtractor::extractSource(const std::filesystem::path& _path)
{
    std::ifstream file(_path);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << _path << "\n";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string sourceCode = buffer.str();

    auto source = std::make_shared<Source>(
        _path.filename().string(), _path, sourceCode, "UTF-8",
        std::filesystem::last_write_time(_path).time_since_epoch().count());

    return source;
}

} // namespace codex
