#include <codex/include_graph.hpp>

#include <algorithm>
#include <queue>

namespace codex
{

void IncludeGraph::build(const std::vector<std::shared_ptr<SourceNode>>& sourceNodes)
{
    m_entries.clear();
    m_filenameLookup.clear();
    m_entries.reserve(sourceNodes.size());

    // Register all files
    for (size_t i = 0; i < sourceNodes.size(); ++i)
    {
        const auto& sn = sourceNodes[i];
        FileEntry entry;
        entry.path = sn->source->path;
        entry.sourceNode = sn;
        m_entries.push_back(std::move(entry));

        // Build filename lookup: map filename and partial paths to indices
        auto pathStr = sn->source->path.generic_string();
        auto filename = sn->source->path.filename().string();
        m_filenameLookup[filename].push_back(i);
        m_filenameLookup[pathStr].push_back(i);

        // Also register with normalized separators
        auto normalized = sn->source->path.lexically_normal().generic_string();
        if (normalized != pathStr)
        {
            m_filenameLookup[normalized].push_back(i);
        }
    }

    // Resolve includes for each file
    for (size_t i = 0; i < m_entries.size(); ++i)
    {
        const auto& sn = m_entries[i].sourceNode;
        auto includerDir = m_entries[i].path.parent_path();

        for (const auto& child : sn->children)
        {
            if (child->kind != NodeKind::IncludeDirective) continue;

            auto* inc = static_cast<const IncludeNode*>(child.get());
            if (inc->isSystem) continue;

            size_t resolved = resolveInclude(inc->path, includerDir);
            if (resolved == static_cast<size_t>(-1)) continue;
            if (resolved == i) continue; // self-include

            // Avoid duplicates
            auto& deps = m_entries[i].dependsOn;
            if (std::find(deps.begin(), deps.end(), resolved) == deps.end())
            {
                deps.push_back(resolved);
                m_entries[resolved].dependedBy.push_back(i);
            }
        }
    }
}

size_t IncludeGraph::resolveInclude(const std::string& includePath,
                                    const std::filesystem::path& includerDir) const
{
    // Try exact path relative to includer directory
    auto candidate = (includerDir / includePath).lexically_normal().generic_string();
    auto it = m_filenameLookup.find(candidate);
    if (it != m_filenameLookup.end() && !it->second.empty())
    {
        return it->second.front();
    }

    // Try matching by filename suffix
    // Extract the filename from the include path
    std::filesystem::path incPath(includePath);
    auto filename = incPath.filename().string();

    it = m_filenameLookup.find(filename);
    if (it == m_filenameLookup.end() || it->second.empty())
    {
        return static_cast<size_t>(-1);
    }

    // If only one match, return it
    if (it->second.size() == 1)
    {
        return it->second.front();
    }

    // Multiple matches: prefer file whose path ends with the include path
    auto includeGeneric = incPath.generic_string();
    size_t bestMatch = static_cast<size_t>(-1);
    size_t bestLen = 0;

    for (size_t idx : it->second)
    {
        auto entryGeneric = m_entries[idx].path.generic_string();
        // Check if the entry path ends with the include path
        if (entryGeneric.size() >= includeGeneric.size())
        {
            auto suffix = entryGeneric.substr(entryGeneric.size() - includeGeneric.size());
            if (suffix == includeGeneric)
            {
                // Prefer same directory, then shortest path
                auto entryDir = m_entries[idx].path.parent_path();
                if (entryDir == includerDir) return idx;

                if (bestMatch == static_cast<size_t>(-1) || entryGeneric.size() < bestLen)
                {
                    bestMatch = idx;
                    bestLen = entryGeneric.size();
                }
            }
        }
    }

    if (bestMatch != static_cast<size_t>(-1)) return bestMatch;

    // Fallback: first match
    return it->second.front();
}

std::vector<size_t> IncludeGraph::topologicalSort() const
{
    // Kahn's algorithm
    size_t n = m_entries.size();
    std::vector<size_t> inDegree(n, 0);

    for (size_t i = 0; i < n; ++i)
    {
        inDegree[i] = m_entries[i].dependsOn.size();
    }

    std::queue<size_t> queue;
    for (size_t i = 0; i < n; ++i)
    {
        if (inDegree[i] == 0) queue.push(i);
    }

    std::vector<size_t> order;
    order.reserve(n);

    while (!queue.empty())
    {
        size_t cur = queue.front();
        queue.pop();
        order.push_back(cur);

        for (size_t dep : m_entries[cur].dependedBy)
        {
            if (--inDegree[dep] == 0) queue.push(dep);
        }
    }

    // Cycle detected: fall back to input order for remaining nodes
    if (order.size() < n)
    {
        for (size_t i = 0; i < n; ++i)
        {
            if (std::find(order.begin(), order.end(), i) == order.end())
            {
                order.push_back(i);
            }
        }
    }

    return order;
}

const std::vector<IncludeGraph::FileEntry>& IncludeGraph::entries() const { return m_entries; }

} // namespace codex
