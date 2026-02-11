#include <codex/models/source.hpp>
#include <codex/models/nodes.hpp>
#include <codex/parser.hpp>

#include <functional>

namespace codex::tests
{

// Helper to create a source from a string
inline std::shared_ptr<Source> makeSource(const std::string& _content,
                                          const std::string& _name = "test.hpp")
{
    return std::make_shared<Source>(_name, std::filesystem::path(_name), _content, "utf-8", 0);
}

inline std::shared_ptr<Source> makeSourceWithPath(const std::string& content,
                                                  const std::string& path,
                                                  const std::string& name = "")
{
    std::string n = name.empty() ? std::filesystem::path(path).filename().string() : name;
    return std::make_shared<Source>(n, std::filesystem::path(path), content, "utf-8", 0);
}

// Helper to parse a single source string
inline std::shared_ptr<SourceNode> parseSingle(const std::string& _content, Parser& _parser)
{
    auto src = makeSource(_content);
    _parser.reset(); // Ensure parser is reset before parsing new source
    return _parser.parse(src).root;
}

inline std::shared_ptr<SourceNode> parseWithPath(const std::string& content,
                                                 const std::string& path, Parser& _parser)
{
    auto src = makeSourceWithPath(content, path);
    _parser.reset(); // Ensure parser is reset before parsing new source
    return _parser.parse(src).root;
}

} // namespace codex::tests
