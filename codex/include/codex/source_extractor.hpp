#pragma once

#include <memory>
#include <vector>
#include <string>
#include <filesystem>

#include "source.hpp"

namespace codex
{

class SourceExtractor
{
   public:
    std::shared_ptr<Source> extractSource(const std::filesystem::path& _path);
};

} // namespace codex
