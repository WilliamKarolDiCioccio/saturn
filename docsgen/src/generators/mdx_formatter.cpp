#include "mdx_formatter.hpp"
#include "cross_linker.hpp"

#include <algorithm>
#include <regex>
#include <sstream>

namespace docsgen
{

std::string MDXFormatter::formatComment(const std::string& rawComment)
{
    if (rawComment.empty()) return "";

    std::istringstream iss(rawComment);
    std::ostringstream oss;
    std::string line;
    bool firstLine = true;
    bool inCodeBlock = false;

    while (std::getline(iss, line))
    {
        // Strip leading whitespace
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos)
        {
            if (!firstLine) oss << "\n";
            firstLine = false;
            continue;
        }
        line = line.substr(start);

        // Strip comment markers
        if (line.starts_with("///"))
            line = line.substr(3);
        else if (line.starts_with("//"))
            line = line.substr(2);
        else if (line.starts_with("/**"))
            line = line.substr(3);
        else if (line.starts_with("/*"))
            line = line.substr(2);
        else if (line.starts_with("*/"))
            continue;
        else if (line.starts_with("*"))
            line = line.substr(1);

        // Strip leading space after marker
        if (!line.empty() && line[0] == ' ') line = line.substr(1);

        // Strip trailing */
        if (line.ends_with("*/"))
        {
            line = line.substr(0, line.size() - 2);
            // Trim trailing whitespace
            while (!line.empty() && std::isspace(static_cast<unsigned char>(line.back())))
                line.pop_back();
        }

        // Track fenced code blocks — escape only outside them
        size_t ws = line.find_first_not_of(" \t");
        bool isCodeFence = ws != std::string::npos && line.substr(ws).starts_with("```");

        if (isCodeFence)
            inCodeBlock = !inCodeBlock;
        else if (!inCodeBlock)
            line = escapeInlineMDX(line);

        if (!firstLine) oss << "\n";
        oss << line;
        firstLine = false;
    }

    return oss.str();
}

std::string MDXFormatter::escapeInlineMDX(const std::string& line)
{
    std::string result;
    result.reserve(line.size() * 1.1);

    bool inInlineCode = false;

    for (char c : line)
    {
        if (c == '`')
        {
            inInlineCode = !inInlineCode;
            result += c;
            continue;
        }

        if (inInlineCode)
        {
            result += c;
            continue;
        }

        switch (c)
        {
            case '<':
                result += "&lt;";
                break;
            case '>':
                result += "&gt;";
                break;
            case '{':
                result += "\\{";
                break;
            case '}':
                result += "\\}";
                break;
            default:
                result += c;
        }
    }

    return result;
}

std::string MDXFormatter::extractBrief(const std::string& rawComment)
{
    std::string formatted = formatComment(rawComment);
    if (formatted.empty()) return "";

    // Look for @brief tag
    size_t briefPos = formatted.find("@brief");
    if (briefPos != std::string::npos)
    {
        size_t start = briefPos + 6;
        while (start < formatted.size() &&
               std::isspace(static_cast<unsigned char>(formatted[start])))
            start++;

        size_t end = formatted.find('\n', start);
        if (end == std::string::npos) end = formatted.size();
        return formatted.substr(start, end - start);
    }

    // Otherwise, return first sentence
    size_t end = formatted.find_first_of(".!?\n");
    if (end != std::string::npos && formatted[end] != '\n') end++;
    return formatted.substr(0, end);
}

std::string MDXFormatter::escapeMDX(const std::string& text)
{
    std::string result;
    result.reserve(text.size() * 1.1);

    for (char c : text)
    {
        switch (c)
        {
            case '<':
                result += "&lt;";
                break;
            case '>':
                result += "&gt;";
                break;
            case '{':
                result += "\\{";
                break;
            case '}':
                result += "\\}";
                break;
            default:
                result += c;
        }
    }

    return result;
}

std::string MDXFormatter::formatTemplateDecl(const std::shared_ptr<codex::Node>& templateDecl)
{
    if (!templateDecl) return "";

    auto* tmpl = dynamic_cast<codex::TemplateNode*>(templateDecl.get());
    if (!tmpl) return "";

    std::ostringstream oss;
    oss << "template <";
    for (size_t i = 0; i < tmpl->parameters.size(); ++i)
    {
        if (i > 0) oss << ", ";
        oss << tmpl->parameters[i].toString();
    }
    oss << ">\n";
    return oss.str();
}

std::string MDXFormatter::formatModifiers(const codex::FunctionNode& fn)
{
    std::ostringstream oss;
    if (fn.isStatic) oss << "static ";
    if (fn.isVirtual) oss << "virtual ";
    if (fn.isInline) oss << "inline ";
    if (fn.isConstexpr) oss << "constexpr ";
    if (fn.isConsteval) oss << "consteval ";
    if (fn.isExplicit) oss << "explicit ";
    return oss.str();
}

std::string MDXFormatter::formatModifiers(const codex::ConstructorNode& ctor)
{
    std::ostringstream oss;
    if (ctor.isInline) oss << "inline ";
    if (ctor.isConstexpr) oss << "constexpr ";
    if (ctor.isExplicit) oss << "explicit ";
    return oss.str();
}

std::string MDXFormatter::formatModifiers(const codex::DestructorNode& dtor)
{
    std::ostringstream oss;
    if (dtor.isVirtual) oss << "virtual ";
    if (dtor.isInline) oss << "inline ";
    if (dtor.isConstexpr) oss << "constexpr ";
    return oss.str();
}

std::string MDXFormatter::formatModifiers(const codex::OperatorNode& op)
{
    std::ostringstream oss;
    if (op.isStatic) oss << "static ";
    if (op.isVirtual) oss << "virtual ";
    if (op.isInline) oss << "inline ";
    if (op.isConstexpr) oss << "constexpr ";
    if (op.isExplicit) oss << "explicit ";
    return oss.str();
}

std::string MDXFormatter::formatModifiers(const codex::VariableNode& var)
{
    std::ostringstream oss;
    if (var.isStatic) oss << "static ";
    if (var.isInline) oss << "inline ";
    if (var.isConstexpr) oss << "constexpr ";
    if (var.isConstinit) oss << "constinit ";
    if (var.isExtern) oss << "extern ";
    if (var.isThreadLocal) oss << "thread_local ";
    return oss.str();
}

std::string MDXFormatter::formatFunctionSignature(const codex::FunctionNode& fn)
{
    std::ostringstream oss;

    oss << formatTemplateDecl(fn.templateDecl);
    oss << formatModifiers(fn);
    oss << fn.returnSignature.toString() << " " << fn.name << "(";

    for (size_t i = 0; i < fn.parameters.size(); ++i)
    {
        if (i > 0) oss << ", ";
        oss << fn.parameters[i].toString();
    }
    oss << ")";

    if (fn.isConst) oss << " const";
    if (fn.isVolatile) oss << " volatile";
    if (fn.isNoexcept) oss << " noexcept";
    if (fn.isOverride) oss << " override";
    if (fn.isFinal) oss << " final";
    if (fn.isPureVirtual) oss << " = 0";

    return oss.str();
}

std::string MDXFormatter::formatConstructorSignature(const codex::ConstructorNode& ctor)
{
    std::ostringstream oss;

    oss << formatTemplateDecl(ctor.templateDecl);
    oss << formatModifiers(ctor);
    oss << ctor.name << "(";

    for (size_t i = 0; i < ctor.parameters.size(); ++i)
    {
        if (i > 0) oss << ", ";
        oss << ctor.parameters[i].toString();
    }
    oss << ")";

    if (ctor.isNoexcept) oss << " noexcept";
    if (ctor.isDefaulted) oss << " = default";
    if (ctor.isDeleted) oss << " = delete";

    return oss.str();
}

std::string MDXFormatter::formatDestructorSignature(const codex::DestructorNode& dtor)
{
    std::ostringstream oss;

    oss << formatModifiers(dtor);
    oss << "~" << dtor.name << "()";

    if (dtor.isNoexcept) oss << " noexcept";
    if (dtor.isDefaulted) oss << " = default";
    if (dtor.isDeleted) oss << " = delete";
    if (dtor.isPureVirtual) oss << " = 0";

    return oss.str();
}

std::string MDXFormatter::formatOperatorSignature(const codex::OperatorNode& op)
{
    std::ostringstream oss;

    oss << formatTemplateDecl(op.templateDecl);
    oss << formatModifiers(op);
    oss << op.returnSignature.toString() << " operator" << op.operatorSymbol << "(";

    for (size_t i = 0; i < op.parameters.size(); ++i)
    {
        if (i > 0) oss << ", ";
        oss << op.parameters[i].toString();
    }
    oss << ")";

    if (op.isConst) oss << " const";
    if (op.isNoexcept) oss << " noexcept";
    if (op.isOverride) oss << " override";
    if (op.isFinal) oss << " final";
    if (op.isPureVirtual) oss << " = 0";

    return oss.str();
}

std::string MDXFormatter::formatVariableSignature(const codex::VariableNode& var)
{
    std::ostringstream oss;

    oss << formatModifiers(var);
    oss << var.typeSignature.toString() << " " << var.name;

    if (!var.defaultValue.empty()) oss << " = " << var.defaultValue;

    return oss.str();
}

std::string MDXFormatter::formatEnumSignature(const codex::EnumNode& en)
{
    std::ostringstream oss;

    oss << "enum";
    if (en.isScoped) oss << " class";
    oss << " " << en.name;
    if (!en.underlyingType.empty()) oss << " : " << en.underlyingType;

    return oss.str();
}

std::string MDXFormatter::formatTypeAliasSignature(const codex::TypeAliasNode& alias)
{
    std::ostringstream oss;

    oss << formatTemplateDecl(alias.templateDecl);
    oss << "using " << alias.aliasName << " = " << alias.targetType;

    return oss.str();
}

std::string MDXFormatter::formatConceptSignature(const codex::ConceptNode& _concept)
{
    std::ostringstream oss;

    oss << formatTemplateDecl(_concept.templateDecl);
    oss << "concept " << _concept.name << " = " << _concept.constraint;

    return oss.str();
}

std::string MDXFormatter::formatParametersTable(const std::vector<codex::FunctionParameter>& params,
                                                const CrossLinker& linker)
{
    if (params.empty()) return "";

    std::ostringstream oss;
    oss << "| Name | Type | Default |\n";
    oss << "|------|------|----------|\n";

    for (const auto& param : params)
    {
        oss << "| `" << (param.name.empty() ? "-" : param.name) << "` | ";
        oss << linker.linkifyTypeSignature(param.typeSignature) << " | ";
        oss << (param.defaultValue.empty() ? "-" : escapeMDX(param.defaultValue)) << " |\n";
    }

    return oss.str();
}

std::string MDXFormatter::formatTemplateParamsTable(
    const std::vector<codex::TemplateParameter>& params)
{
    if (params.empty()) return "";

    std::ostringstream oss;
    oss << "| Parameter | Kind | Default |\n";
    oss << "|-----------|------|----------|\n";

    for (const auto& param : params)
    {
        std::string kind;
        switch (param.paramKind)
        {
            case codex::TemplateParameterKind::Type:
                kind = "type";
                break;
            case codex::TemplateParameterKind::NonType:
                kind = "non-type";
                break;
            case codex::TemplateParameterKind::TemplateTemplate:
                kind = "template";
                break;
        }

        oss << "| `" << param.toString() << "` | " << kind << " | ";
        oss << (param.defaultValue.empty() ? "-" : escapeMDX(param.defaultValue)) << " |\n";
    }

    return oss.str();
}

std::string MDXFormatter::formatEnumValuesTable(
    const std::vector<std::shared_ptr<codex::Node>>& enumerators)
{
    if (enumerators.empty()) return "";

    std::ostringstream oss;
    oss << "| Value | Description |\n";
    oss << "|-------|-------------|\n";

    for (const auto& node : enumerators)
    {
        auto* spec = dynamic_cast<codex::EnumSpecifierNode*>(node.get());
        if (!spec) continue;

        oss << "| `" << spec->name;
        if (!spec->value.empty()) oss << " = " << escapeMDX(spec->value);
        oss << "` | ";

        if (spec->comment)
        {
            auto* comment = dynamic_cast<codex::CommentNode*>(spec->comment.get());
            if (comment) oss << formatComment(comment->text);
        }
        oss << " |\n";
    }

    return oss.str();
}

std::string MDXFormatter::formatSourceLocation(const codex::SourceLocation& loc)
{
    std::ostringstream oss;
    oss << loc.filePath.filename().string() << ":" << loc.startLine;
    return oss.str();
}

} // namespace docsgen
