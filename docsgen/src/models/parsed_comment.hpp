#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace docsgen
{

struct ParsedComment
{
    std::string brief;
    std::string description;
    std::unordered_map<std::string, std::string> params;
    std::unordered_map<std::string, std::string> tparams;
    std::string returns;
    std::vector<std::string> notes;
    std::vector<std::string> seeAlso;
};

} // namespace docsgen
