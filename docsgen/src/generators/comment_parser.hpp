#pragma once

#include <string>

#include "../models/parsed_comment.hpp"

namespace docsgen
{

class CommentParser
{
   public:
    static ParsedComment parse(const std::string& rawComment);

   private:
    static std::string stripCommentMarkers(const std::string& line);
    static std::string extractFirstSentence(const std::string& text);
};

} // namespace docsgen
