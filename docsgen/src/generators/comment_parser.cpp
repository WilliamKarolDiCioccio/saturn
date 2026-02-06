#include "comment_parser.hpp"
#include "mdx_formatter.hpp"

#include <sstream>

namespace docsgen
{

std::string CommentParser::stripCommentMarkers(const std::string& line)
{
    // Strip leading whitespace
    size_t start = line.find_first_not_of(" \t");
    if (start == std::string::npos) return "";

    std::string stripped = line.substr(start);

    // Strip comment markers (same logic as MDXFormatter::formatComment)
    if (stripped.starts_with("///"))
        stripped = stripped.substr(3);
    else if (stripped.starts_with("//"))
        stripped = stripped.substr(2);
    else if (stripped.starts_with("/**"))
        stripped = stripped.substr(3);
    else if (stripped.starts_with("/*"))
        stripped = stripped.substr(2);
    else if (stripped.starts_with("*/"))
        return "";
    else if (stripped.starts_with("*"))
        stripped = stripped.substr(1);

    // Strip leading space after marker
    if (!stripped.empty() && stripped[0] == ' ') stripped = stripped.substr(1);

    // Strip trailing */
    if (stripped.ends_with("*/"))
    {
        stripped = stripped.substr(0, stripped.size() - 2);
        while (!stripped.empty() && std::isspace(static_cast<unsigned char>(stripped.back())))
            stripped.pop_back();
    }

    return stripped;
}

std::string CommentParser::extractFirstSentence(const std::string& text)
{
    if (text.empty()) return "";

    size_t end = text.find_first_of(".!?\n");
    if (end != std::string::npos && text[end] != '\n') end++;
    return text.substr(0, end);
}

ParsedComment CommentParser::parse(const std::string& rawComment)
{
    ParsedComment result;
    if (rawComment.empty()) return result;

    std::istringstream iss(rawComment);
    std::string rawLine;
    bool inCodeBlock = false;

    // Current tag state
    enum class TagKind
    {
        None,
        Brief,
        Param,
        Tparam,
        Returns,
        Note,
        See,
        Description
    };
    TagKind currentTag = TagKind::Description;
    std::string currentParamName;

    std::ostringstream descStream;
    std::ostringstream briefStream;
    std::ostringstream returnsStream;
    std::ostringstream tagStream; // accumulator for current multi-line tag

    auto flushTag = [&]()
    {
        std::string text = tagStream.str();
        // Trim trailing whitespace
        while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back())))
            text.pop_back();

        switch (currentTag)
        {
            case TagKind::Brief:
                briefStream << text;
                break;
            case TagKind::Param:
                if (!currentParamName.empty()) result.params[currentParamName] = text;
                break;
            case TagKind::Tparam:
                if (!currentParamName.empty()) result.tparams[currentParamName] = text;
                break;
            case TagKind::Returns:
                returnsStream << text;
                break;
            case TagKind::Note:
                if (!text.empty()) result.notes.push_back(text);
                break;
            case TagKind::See:
                if (!text.empty()) result.seeAlso.push_back(text);
                break;
            case TagKind::Description:
            case TagKind::None:
                break;
        }

        tagStream.str("");
        tagStream.clear();
        currentTag = TagKind::None;
        currentParamName.clear();
    };

    bool firstDescLine = true;

    while (std::getline(iss, rawLine))
    {
        std::string line = stripCommentMarkers(rawLine);

        // Track fenced code blocks
        size_t ws = line.find_first_not_of(" \t");
        bool isCodeFence = ws != std::string::npos && line.substr(ws).starts_with("```");

        if (isCodeFence)
        {
            inCodeBlock = !inCodeBlock;
            // Pass code fences through to current accumulator
            if (currentTag == TagKind::Description || currentTag == TagKind::None)
            {
                if (!firstDescLine) descStream << "\n";
                descStream << line;
                firstDescLine = false;
            }
            else
            {
                if (!tagStream.str().empty()) tagStream << "\n";
                tagStream << line;
            }
            continue;
        }

        if (inCodeBlock)
        {
            // Never parse @ inside code blocks
            if (currentTag == TagKind::Description || currentTag == TagKind::None)
            {
                if (!firstDescLine) descStream << "\n";
                descStream << line;
                firstDescLine = false;
            }
            else
            {
                if (!tagStream.str().empty()) tagStream << "\n";
                tagStream << line;
            }
            continue;
        }

        // Escape MDX outside code blocks
        line = MDXFormatter::escapeInlineMDX(line);

        // Check for tags
        if (line.starts_with("@brief ") || line == "@brief")
        {
            flushTag();
            currentTag = TagKind::Brief;
            std::string rest = line.size() > 7 ? line.substr(7) : "";
            tagStream << rest;
            continue;
        }

        if (line.starts_with("@param "))
        {
            flushTag();
            currentTag = TagKind::Param;
            std::string rest = line.substr(7);
            // Extract param name (first word)
            size_t nameEnd = rest.find_first_of(" \t");
            if (nameEnd != std::string::npos)
            {
                currentParamName = rest.substr(0, nameEnd);
                // Skip whitespace after name
                size_t descStart = rest.find_first_not_of(" \t", nameEnd);
                if (descStart != std::string::npos) tagStream << rest.substr(descStart);
            }
            else
            {
                currentParamName = rest;
            }
            continue;
        }

        if (line.starts_with("@tparam "))
        {
            flushTag();
            currentTag = TagKind::Tparam;
            std::string rest = line.substr(8);
            size_t nameEnd = rest.find_first_of(" \t");
            if (nameEnd != std::string::npos)
            {
                currentParamName = rest.substr(0, nameEnd);
                size_t descStart = rest.find_first_not_of(" \t", nameEnd);
                if (descStart != std::string::npos) tagStream << rest.substr(descStart);
            }
            else
            {
                currentParamName = rest;
            }
            continue;
        }

        if (line.starts_with("@return ") || line.starts_with("@returns ") || line == "@return" ||
            line == "@returns")
        {
            flushTag();
            currentTag = TagKind::Returns;
            size_t tagLen = line.starts_with("@returns") ? 8 : 7;
            std::string rest = line.size() > tagLen + 1 ? line.substr(tagLen + 1) : "";
            tagStream << rest;
            continue;
        }

        if (line.starts_with("@note ") || line == "@note")
        {
            flushTag();
            currentTag = TagKind::Note;
            std::string rest = line.size() > 6 ? line.substr(6) : "";
            tagStream << rest;
            continue;
        }

        if (line.starts_with("@see ") || line == "@see")
        {
            flushTag();
            currentTag = TagKind::See;
            std::string rest = line.size() > 5 ? line.substr(5) : "";
            tagStream << rest;
            continue;
        }

        // Continuation line or description
        if (currentTag == TagKind::None || currentTag == TagKind::Description)
        {
            currentTag = TagKind::Description;
            if (!firstDescLine) descStream << "\n";
            descStream << line;
            firstDescLine = false;
        }
        else
        {
            // Blank line ends @brief (Doxygen first-paragraph rule)
            if (line.empty() && currentTag == TagKind::Brief)
            {
                flushTag();
                currentTag = TagKind::Description;
                continue;
            }

            // Multi-line continuation of current tag
            if (!tagStream.str().empty()) tagStream << " ";
            tagStream << line;
        }
    }

    // Flush last tag
    flushTag();

    result.description = descStream.str();
    result.brief = briefStream.str();
    result.returns = returnsStream.str();

    // If no @brief, extract first sentence from description
    if (result.brief.empty()) result.brief = extractFirstSentence(result.description);

    return result;
}

} // namespace docsgen
