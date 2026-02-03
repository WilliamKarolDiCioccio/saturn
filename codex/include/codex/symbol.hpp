#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "nodes.hpp"

namespace codex
{

struct SymbolID
{
    uint32_t value = 0;

    static constexpr SymbolID invalid() { return SymbolID{0}; }

    bool operator==(const SymbolID& other) const = default;
    auto operator<=>(const SymbolID& other) const = default;

    explicit operator bool() const { return value != 0; }
};

enum struct SymbolKind
{
    Namespace,
    Class,
    Struct,
    Union,
    Enum,
    EnumValue,
    Function,
    Constructor,
    Destructor,
    Operator,
    Variable,
    TypeAlias,
    Concept,
    ObjectLikeMacro,
    FunctionLikeMacro
};

inline std::string symbolKindToString(SymbolKind kind)
{
    switch (kind)
    {
        case SymbolKind::Namespace:
            return "Namespace";
        case SymbolKind::Class:
            return "Class";
        case SymbolKind::Struct:
            return "Struct";
        case SymbolKind::Union:
            return "Union";
        case SymbolKind::Enum:
            return "Enum";
        case SymbolKind::EnumValue:
            return "EnumValue";
        case SymbolKind::Function:
            return "Function";
        case SymbolKind::Constructor:
            return "Constructor";
        case SymbolKind::Destructor:
            return "Destructor";
        case SymbolKind::Operator:
            return "Operator";
        case SymbolKind::Variable:
            return "Variable";
        case SymbolKind::TypeAlias:
            return "TypeAlias";
        case SymbolKind::Concept:
            return "Concept";
        case SymbolKind::ObjectLikeMacro:
            return "ObjectLikeMacro";
        case SymbolKind::FunctionLikeMacro:
            return "FunctionLikeMacro";
        default:
            return "Unknown";
    }
}

struct SourceLocation
{
    std::filesystem::path filePath;
    uint32_t startLine = 0;
    uint32_t startColumn = 0;
    uint32_t endLine = 0;
    uint32_t endColumn = 0;
};

struct Symbol
{
    SymbolID id;
    SymbolKind symbolKind;
    std::string name;
    std::string qualifiedName;
    std::string signature;
    SourceLocation location;
    AccessSpecifier access = AccessSpecifier::None;
    std::shared_ptr<Node> node;

    // Cross-refs (populated in linking phase)
    std::vector<SymbolID> baseClasses;
    std::vector<SymbolID> derivedClasses;
    std::vector<SymbolID> referencedTypes;
    std::vector<SymbolID> referencedBy;
    SymbolID parentSymbol = SymbolID::invalid();
    std::vector<SymbolID> childSymbols;

    bool isForwardDeclaration = false;
};

} // namespace codex

template <>
struct std::hash<codex::SymbolID>
{
    size_t operator()(const codex::SymbolID& id) const noexcept
    {
        return std::hash<uint32_t>{}(id.value);
    }
};
