#include "cross_linker.hpp"

#include <algorithm>
#include <regex>
#include <sstream>

namespace docsgen
{

void CrossLinker::buildIndex(const codex::SymbolsDatabase& db)
{
    m_qualifiedToFilename.clear();

    for (const codex::Symbol* sym : db.allSymbols())
    {
        if (!sym || sym->isForwardDeclaration) continue;

        switch (sym->symbolKind)
        {
            case codex::SymbolKind::Class:
            case codex::SymbolKind::Struct:
            case codex::SymbolKind::Enum:
            case codex::SymbolKind::Function:
            case codex::SymbolKind::TypeAlias:
            case codex::SymbolKind::Concept:
            case codex::SymbolKind::Operator:
            {
                if (sym->access == codex::AccessSpecifier::Private ||
                    sym->access == codex::AccessSpecifier::Protected)
                    continue;

                std::string filename = qualifiedToFilename(sym->qualifiedName);
                m_qualifiedToFilename[sym->qualifiedName] = filename;
                m_qualifiedToFilename[sym->name] = filename;
                break;
            }
            default:
                break;
        }
    }
}

std::string CrossLinker::qualifiedToFilename(const std::string& qualifiedName)
{
    std::string result = qualifiedName;

    // Strip template params from filename
    size_t templateStart = result.find('<');
    if (templateStart != std::string::npos) result = result.substr(0, templateStart);

    // Replace :: with _
    size_t pos = 0;
    while ((pos = result.find("::", pos)) != std::string::npos)
    {
        result.replace(pos, 2, "_");
        pos += 1;
    }

    // Handle operators
    if (result.find("operator") != std::string::npos)
    {
        size_t opPos = result.find("operator");
        std::string prefix = result.substr(0, opPos);
        std::string opPart = result.substr(opPos);
        result = prefix + sanitizeOperator(opPart);
    }

    return result;
}

std::string CrossLinker::sanitizeOperator(const std::string& op)
{
    static const std::unordered_map<std::string, std::string> opMap = {
        {"operator==", "operator_eq_eq"},
        {"operator!=", "operator_neq"},
        {"operator<", "operator_lt"},
        {"operator>", "operator_gt"},
        {"operator<=", "operator_lte"},
        {"operator>=", "operator_gte"},
        {"operator<=>", "operator_spaceship"},
        {"operator+", "operator_plus"},
        {"operator-", "operator_minus"},
        {"operator*", "operator_mul"},
        {"operator/", "operator_div"},
        {"operator%", "operator_mod"},
        {"operator++", "operator_inc"},
        {"operator--", "operator_dec"},
        {"operator&", "operator_amp"},
        {"operator|", "operator_pipe"},
        {"operator^", "operator_xor"},
        {"operator~", "operator_not"},
        {"operator!", "operator_lnot"},
        {"operator&&", "operator_land"},
        {"operator||", "operator_lor"},
        {"operator<<", "operator_lshift"},
        {"operator>>", "operator_rshift"},
        {"operator=", "operator_assign"},
        {"operator+=", "operator_plus_assign"},
        {"operator-=", "operator_minus_assign"},
        {"operator*=", "operator_mul_assign"},
        {"operator/=", "operator_div_assign"},
        {"operator%=", "operator_mod_assign"},
        {"operator&=", "operator_amp_assign"},
        {"operator|=", "operator_pipe_assign"},
        {"operator^=", "operator_xor_assign"},
        {"operator<<=", "operator_lshift_assign"},
        {"operator>>=", "operator_rshift_assign"},
        {"operator[]", "operator_subscript"},
        {"operator()", "operator_call"},
        {"operator->", "operator_arrow"},
        {"operator->*", "operator_arrow_ptr"},
        {"operator,", "operator_comma"},
        {"operator new", "operator_new"},
        {"operator delete", "operator_delete"},
        {"operator new[]", "operator_new_arr"},
        {"operator delete[]", "operator_delete_arr"},
        {"operator bool", "operator_bool"},
    };

    auto it = opMap.find(op);
    if (it != opMap.end()) return it->second;

    // Fallback: replace non-alphanumeric chars
    std::string result;
    for (char c : op)
    {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_')
            result += c;
        else
            result += '_';
    }
    return result;
}

std::string CrossLinker::findFilename(const std::string& name) const
{
    auto it = m_qualifiedToFilename.find(name);
    return it != m_qualifiedToFilename.end() ? it->second : "";
}

std::string CrossLinker::linkify(const std::string& typeName) const
{
    // Strip qualifiers for lookup
    std::string cleanType = typeName;

    // Remove const/volatile/mutable
    static const std::regex qualifiers(R"(\b(const|volatile|mutable)\s*)");
    cleanType = std::regex_replace(cleanType, qualifiers, "");

    // Remove pointer/reference
    while (!cleanType.empty() && (cleanType.back() == '*' || cleanType.back() == '&'))
        cleanType.pop_back();

    // Trim whitespace
    while (!cleanType.empty() && std::isspace(static_cast<unsigned char>(cleanType.front())))
        cleanType.erase(0, 1);
    while (!cleanType.empty() && std::isspace(static_cast<unsigned char>(cleanType.back())))
        cleanType.pop_back();

    return linkifyBaseType(cleanType);
}

std::string CrossLinker::linkifyBaseType(const std::string& baseType) const
{
    // Check for template type
    size_t templateStart = baseType.find('<');
    if (templateStart != std::string::npos)
    {
        std::string outerType = baseType.substr(0, templateStart);

        // Parse template args (simplified - doesn't handle nested templates properly)
        std::string argsStr = baseType.substr(templateStart + 1);
        if (!argsStr.empty() && argsStr.back() == '>') argsStr.pop_back();

        // Linkify outer type
        std::string result = linkifyBaseType(outerType);
        result += "&lt;";

        // Split and linkify args (basic comma split)
        std::istringstream iss(argsStr);
        std::string arg;
        bool first = true;
        int depth = 0;
        std::string currentArg;

        for (char c : argsStr)
        {
            if (c == '<')
                depth++;
            else if (c == '>')
                depth--;

            if (c == ',' && depth == 0)
            {
                if (!first) result += ", ";
                first = false;
                // Trim and linkify
                while (!currentArg.empty() &&
                       std::isspace(static_cast<unsigned char>(currentArg.front())))
                    currentArg.erase(0, 1);
                while (!currentArg.empty() &&
                       std::isspace(static_cast<unsigned char>(currentArg.back())))
                    currentArg.pop_back();
                result += linkifyBaseType(currentArg);
                currentArg.clear();
            }
            else
            {
                currentArg += c;
            }
        }

        if (!currentArg.empty())
        {
            if (!first) result += ", ";
            while (!currentArg.empty() &&
                   std::isspace(static_cast<unsigned char>(currentArg.front())))
                currentArg.erase(0, 1);
            while (!currentArg.empty() &&
                   std::isspace(static_cast<unsigned char>(currentArg.back())))
                currentArg.pop_back();
            result += linkifyBaseType(currentArg);
        }

        result += "&gt;";
        return result;
    }

    // Look up in index
    auto it = m_qualifiedToFilename.find(baseType);
    if (it != m_qualifiedToFilename.end())
    {
        return "[" + baseType + "](/saturn/api_reference/" + it->second + ")";
    }

    // Not found, return as-is
    return baseType;
}

std::string CrossLinker::linkifyTypeSignature(const codex::TypeSignature& ts) const
{
    std::ostringstream oss;

    if (ts.isConst) oss << "const ";
    if (ts.isVolatile) oss << "volatile ";
    if (ts.isMutable) oss << "mutable ";

    // Linkify base type
    oss << linkifyBaseType(ts.baseType);

    // Template args
    if (!ts.templateArgs.empty())
    {
        oss << "&lt;";
        for (size_t i = 0; i < ts.templateArgs.size(); ++i)
        {
            if (i > 0) oss << ", ";
            const auto& arg = ts.templateArgs[i];
            if (!arg.typeSignature.baseType.empty())
                oss << linkifyTypeSignature(arg.typeSignature);
            else if (!arg.value.empty())
                oss << arg.value;
            else if (!arg.keyword.empty())
                oss << arg.keyword;
        }
        oss << "&gt;";
    }

    if (ts.isPointer) oss << "*";
    if (ts.isLValueRef) oss << "&amp;";
    if (ts.isRValueRef) oss << "&amp;&amp;";

    return oss.str();
}

} // namespace docsgen
