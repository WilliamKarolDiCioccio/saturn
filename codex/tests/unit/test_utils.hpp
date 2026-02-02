#include <codex/source.hpp>
#include <codex/nodes.hpp>
#include <codex/parser.hpp>

namespace codex::tests
{

// Helper to create a source from a string
inline std::shared_ptr<Source> makeSource(const std::string& _content,
                                          const std::string& _name = "test.hpp")
{
    return std::make_shared<Source>(_name, std::filesystem::path(_name), _content, "utf-8", 0);
}

// Helper to parse a single source string
inline std::shared_ptr<SourceNode> parseSingle(const std::string& _content)
{
    auto src = makeSource(_content);
    Parser parser;
    auto result = parser.parse(src);
    return result;
}

inline std::function<void(TSNode, int)> dumpAST(TSNode _node, int _depth, const std::string& _code)
{
    std::string indent(_depth * 2, ' ');
    const char* type = ts_node_type(_node);
    bool named = ts_node_is_named(_node);
    uint32_t start = ts_node_start_byte(_node);
    uint32_t end = ts_node_end_byte(_node);
    std::string text = _code.substr(start, end - start);
    if (text.size() > 60) text = text.substr(0, 57) + "...";

    std::cout << indent << (named ? "" : "[anon] ") << type << " \"" << text << "\"" << std::endl;

    uint32_t count = ts_node_child_count(_node);
    for (uint32_t i = 0; i < count; ++i) dumpAST(ts_node_child(_node, i), _depth + 1, _code);
};

} // namespace codex::tests
