#pragma once

#include <string>

namespace docsgen
{

struct MDXFile
{
    std::string filename;
    std::string title;
    std::string description;
    std::string content;
};

} // namespace docsgen
