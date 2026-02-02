#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <filesystem>
#include <memory>
#include <stack>

#include "source.hpp"

namespace codex
{

class Preprocessor
{
   private:
    std::shared_ptr<Source> m_source;
    std::unordered_map<std::string, std::string> m_defines;
    std::unordered_set<std::string> m_definedMacros;
    std::stack<bool> m_conditionalStack;

   private:
    std::string processLine(const std::string& line);
    bool isDefineLine(const std::string& line, const std::string& macroName);
    bool isConditionalDirective(const std::string& line);
    bool handleConditionalDirective(const std::string& line);
    bool evaluateCondition(const std::string& condition);
    bool isCurrentlyEnabled() const;

   public:
    Preprocessor();

   public:
    void expand(const std::shared_ptr<Source>& _source,
                const std::unordered_map<std::string, std::string>& _defines = {},
                const std::unordered_set<std::string>& _definedMacros = {});
    void reset();
};

} // namespace codex
