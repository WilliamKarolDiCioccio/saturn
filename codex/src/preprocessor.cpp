#include "codex/preprocessor.hpp"

#include <future>
#include <iostream>
#include <sstream>
#include <regex>
#include <stack>

namespace codex
{

/////////////////////////////////////////////////////////////////////////////////////////////
// Preprocessor lifecycle
/////////////////////////////////////////////////////////////////////////////////////////////

Preprocessor::Preprocessor()
{
    // Also add all defines to definedMacros (they exist)
    for (const auto& [name, _] : m_defines)
    {
        m_definedMacros.insert(name);
    }
}

void Preprocessor::expand(const std::shared_ptr<Source>& _source,
                          const std::unordered_map<std::string, std::string>& _defines,
                          const std::unordered_set<std::string>& _definedMacros)
{
    m_source = _source;
    m_defines = _defines;
    m_definedMacros = _definedMacros;

    if (!m_source || m_source->content.empty())
    {
        return;
    }

    std::istringstream input(m_source->content);
    std::ostringstream output;
    std::string line;

    // Process line by line
    bool firstLine = true;
    while (std::getline(input, line))
    {
        if (!firstLine)
        {
            output << '\n';
        }
        firstLine = false;

        output << processLine(line);
    }

    m_source->content = output.str();
}

void Preprocessor::reset()
{
    m_source = nullptr;
    m_defines.clear();
    m_definedMacros.clear();

    while (!m_conditionalStack.empty()) m_conditionalStack.pop();
}

bool Preprocessor::isDefineLine(const std::string& line, const std::string& macroName)
{
    std::regex defineRegex(R"(^\s*#\s*define\s+)" + macroName + R"(\s)");
    return std::regex_search(line, defineRegex);
}

bool Preprocessor::isConditionalDirective(const std::string& line)
{
    std::regex condRegex(R"(^\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b)");
    return std::regex_search(line, condRegex);
}

bool Preprocessor::isCurrentlyEnabled() const
{
    if (m_conditionalStack.empty()) return true;

    // Check if all levels of the stack are true
    std::stack<bool> copy = m_conditionalStack;
    while (!copy.empty())
    {
        if (!copy.top()) return false;
        copy.pop();
    }
    return true;
}

bool Preprocessor::evaluateCondition(const std::string& condition)
{
    std::string cond = condition;

    // Trim whitespace
    cond.erase(0, cond.find_first_not_of(" \t"));
    cond.erase(cond.find_last_not_of(" \t") + 1);

    // Handle literal 0 and 1
    if (cond == "0") return false;
    if (cond == "1") return true;

    // Handle defined(MACRO) or defined MACRO
    std::regex definedParenRegex(R"(defined\s*\(\s*(\w+)\s*\))");
    std::regex definedNoParenRegex(R"(defined\s+(\w+))");
    std::smatch match;

    // Replace all defined(MACRO) with 1 or 0
    std::string processed = cond;
    while (std::regex_search(processed, match, definedParenRegex))
    {
        std::string macro = match[1];
        bool isDefined = m_definedMacros.count(macro) > 0;
        processed = match.prefix().str() + (isDefined ? "1" : "0") + match.suffix().str();
    }
    while (std::regex_search(processed, match, definedNoParenRegex))
    {
        std::string macro = match[1];
        bool isDefined = m_definedMacros.count(macro) > 0;
        processed = match.prefix().str() + (isDefined ? "1" : "0") + match.suffix().str();
    }

    // Now evaluate boolean expressions with &&, ||, !
    // Simple recursive descent for: expr = term ((&&|||) term)*
    // term = !term | (expr) | 0 | 1 | MACRO

    // For simplicity, we only handle basic cases:
    // - Single identifier (check if defined)
    // - 0 or 1
    // - !expr
    // - expr && expr
    // - expr || expr

    // Remove spaces for easier parsing
    processed.erase(std::remove(processed.begin(), processed.end(), ' '), processed.end());
    processed.erase(std::remove(processed.begin(), processed.end(), '\t'), processed.end());

    // Simple evaluation
    std::function<bool(const std::string&, size_t&)> parseExpr;
    std::function<bool(const std::string&, size_t&)> parseTerm;
    std::function<bool(const std::string&, size_t&)> parseFactor;

    parseFactor = [&](const std::string& s, size_t& pos) -> bool
    {
        if (pos >= s.size()) return false;

        // Handle !
        if (s[pos] == '!')
        {
            ++pos;
            return !parseFactor(s, pos);
        }

        // Handle parentheses
        if (s[pos] == '(')
        {
            ++pos;
            bool result = parseExpr(s, pos);
            if (pos < s.size() && s[pos] == ')') ++pos;
            return result;
        }

        // Handle 0 or 1
        if (s[pos] == '0')
        {
            ++pos;
            return false;
        }
        if (s[pos] == '1')
        {
            ++pos;
            return true;
        }

        // Handle identifier (macro name) - check if defined
        std::string ident;
        while (pos < s.size() && (std::isalnum(s[pos]) || s[pos] == '_'))
        {
            ident += s[pos++];
        }
        if (!ident.empty())
        {
            return m_definedMacros.count(ident) > 0;
        }

        return false;
    };

    parseTerm = [&](const std::string& s, size_t& pos) -> bool
    {
        bool left = parseFactor(s, pos);

        while (pos + 1 < s.size() && s[pos] == '&' && s[pos + 1] == '&')
        {
            pos += 2;
            bool right = parseFactor(s, pos);
            left = left && right;
        }

        return left;
    };

    parseExpr = [&](const std::string& s, size_t& pos) -> bool
    {
        bool left = parseTerm(s, pos);

        while (pos + 1 < s.size() && s[pos] == '|' && s[pos + 1] == '|')
        {
            pos += 2;
            bool right = parseTerm(s, pos);
            left = left || right;
        }

        return left;
    };

    size_t pos = 0;
    return parseExpr(processed, pos);
}

bool Preprocessor::handleConditionalDirective(const std::string& line)
{
    std::smatch match;

    // #if condition
    std::regex ifRegex(R"(^\s*#\s*if\s+(.+)$)");
    if (std::regex_match(line, match, ifRegex))
    {
        bool result = isCurrentlyEnabled() && evaluateCondition(match[1]);
        m_conditionalStack.push(result);
        return true;
    }

    // #ifdef MACRO
    std::regex ifdefRegex(R"(^\s*#\s*ifdef\s+(\w+))");
    if (std::regex_match(line, match, ifdefRegex))
    {
        bool result = isCurrentlyEnabled() && m_definedMacros.count(match[1]) > 0;
        m_conditionalStack.push(result);
        return true;
    }

    // #ifndef MACRO
    std::regex ifndefRegex(R"(^\s*#\s*ifndef\s+(\w+))");
    if (std::regex_match(line, match, ifndefRegex))
    {
        bool result = isCurrentlyEnabled() && m_definedMacros.count(match[1]) == 0;
        m_conditionalStack.push(result);
        return true;
    }

    // #elif condition
    std::regex elifRegex(R"(^\s*#\s*elif\s+(.+)$)");
    if (std::regex_match(line, match, elifRegex))
    {
        if (!m_conditionalStack.empty())
        {
            // If current block was true, elif is false; otherwise evaluate
            bool wasEnabled = m_conditionalStack.top();
            m_conditionalStack.pop();

            // Check parent context
            bool parentEnabled = isCurrentlyEnabled();

            // elif only activates if previous if/elif was false and condition is true
            bool result = parentEnabled && !wasEnabled && evaluateCondition(match[1]);
            m_conditionalStack.push(wasEnabled || result); // Track if any branch was taken
        }
        return true;
    }

    // #else
    std::regex elseRegex(R"(^\s*#\s*else\b)");
    if (std::regex_search(line, elseRegex))
    {
        if (!m_conditionalStack.empty())
        {
            bool wasEnabled = m_conditionalStack.top();
            m_conditionalStack.pop();

            // Check parent context
            bool parentEnabled = isCurrentlyEnabled();

            // else is enabled only if parent is enabled and no previous branch was taken
            m_conditionalStack.push(parentEnabled && !wasEnabled);
        }
        return true;
    }

    // #endif
    std::regex endifRegex(R"(^\s*#\s*endif\b)");
    if (std::regex_search(line, endifRegex))
    {
        if (!m_conditionalStack.empty())
        {
            m_conditionalStack.pop();
        }
        return true;
    }

    return false;
}

std::string Preprocessor::processLine(const std::string& line)
{
    // Handle conditional directives first
    if (isConditionalDirective(line))
    {
        handleConditionalDirective(line);
        // Keep the directive line but comment it out for debugging
        return "// [preprocessor] " + line;
    }

    // If we're in a disabled conditional block, comment out the line
    if (!isCurrentlyEnabled())
    {
        return "// [disabled] " + line;
    }

    // Handle #define - add to our set of defined macros
    std::regex defineRegex(R"(^\s*#\s*define\s+(\w+))");
    std::smatch match;
    if (std::regex_search(line, match, defineRegex))
    {
        m_definedMacros.insert(match[1]);
    }

    // Handle #undef - remove from defined macros
    std::regex undefRegex(R"(^\s*#\s*undef\s+(\w+))");
    if (std::regex_search(line, match, undefRegex))
    {
        m_definedMacros.erase(match[1]);
    }

    std::string result = line;

    // Process each define (macro replacement)
    for (const auto& [macroName, replacement] : m_defines)
    {
        // Skip if this line defines the macro
        if (isDefineLine(line, macroName))
        {
            continue;
        }

        // Create regex with word boundaries to match only complete identifiers
        std::string pattern = R"(\b)" + macroName + R"(\b)";
        std::regex macroRegex(pattern);

        // Replace all occurrences of the macro with its replacement
        result = std::regex_replace(result, macroRegex, replacement);
    }

    return result;
}

} // namespace codex
