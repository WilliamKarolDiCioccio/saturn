#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "nodes.hpp"

namespace codex
{

class IncludeGraph
{
   public:
    struct FileEntry
    {
        std::filesystem::path path;
        std::shared_ptr<SourceNode> sourceNode;
        std::vector<size_t> dependsOn;
        std::vector<size_t> dependedBy;
    };

    void build(const std::vector<std::shared_ptr<SourceNode>>& sourceNodes);
    std::vector<size_t> topologicalSort() const;
    const std::vector<FileEntry>& entries() const;

   private:
    std::vector<FileEntry> m_entries;
    std::unordered_map<std::string, std::vector<size_t>> m_filenameLookup;

    size_t resolveInclude(const std::string& includePath,
                          const std::filesystem::path& includerDir) const;
};

} // namespace codex
