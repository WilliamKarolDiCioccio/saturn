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
    std::ifstream file(m_path);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << m_path << "\n";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    m_sourceCode = buffer.str();

    auto source = std::make_shared<Source>(
        m_path.filename().string(), m_path, m_sourceCode, "UTF-8",
        std::filesystem::last_write_time(m_path).time_since_epoch().count());

    return source;
}

} // namespace codex
