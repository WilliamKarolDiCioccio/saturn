#pragma once

#include <string>
#include <filesystem>

namespace codex
{

struct Source
{
    std::string name;
    std::filesystem::path path;
    std::string content;
    std::string encoding;
    double lastModifiedTime;

    Source(std::string _name, std::filesystem::path _path, std::string _content,
           std::string _encoding, double _lastModifiedTime)
        : name(std::move(_name)),
          path(std::move(_path)),
          content(std::move(_content)),
          encoding(std::move(_encoding)),
          lastModifiedTime(_lastModifiedTime) {};
};

} // namespace codex
