extern "C"
{
    const TSLanguage* tree_sitter_cpp();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// TS AST helpers
/////////////////////////////////////////////////////////////////////////////////////////////

[[nodiscard]] inline std::tuple<TSPoint, TSPoint> getPositionData(const TSNode& _node)
{
    TSPoint start = ts_node_start_point(_node);
    TSPoint end = ts_node_end_point(_node);

    return {start, end};
}

[[nodiscard]] inline std::string getNodeText(const TSNode& _node, const std::string& _source)
{
    uint32_t startByte = ts_node_start_byte(_node);
    uint32_t endByte = ts_node_end_byte(_node);

    if (startByte > endByte || endByte > _source.size())
    {
        throw std::runtime_error("Invalid node position data");
    }

    return _source.substr(startByte, endByte - startByte);
}
