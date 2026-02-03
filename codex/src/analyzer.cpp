#include <codex/analyzer.hpp>

#include <algorithm>
#include <unordered_set>

namespace codex
{

/////////////////////////////////////////////////////////////////////////////////////////////
// Public API
/////////////////////////////////////////////////////////////////////////////////////////////

AnalysisResult Analyzer::analyze(const std::vector<std::shared_ptr<SourceNode>>& sourceNodes)
{
    AnalysisResult result;

    // Phase 1: Build include graph + topo sort
    result.includeGraph = buildIncludeGraph(sourceNodes);
    auto order = result.includeGraph.topologicalSort();

    // Phase 2: Index symbols in topo order
    indexSymbols(result.includeGraph, order, result.database);

    // Phase 3: Link cross-references
    linkCrossReferences(result.database);

    return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Phase 1
/////////////////////////////////////////////////////////////////////////////////////////////

IncludeGraph Analyzer::buildIncludeGraph(
    const std::vector<std::shared_ptr<SourceNode>>& sourceNodes)
{
    IncludeGraph graph;
    graph.build(sourceNodes);
    return graph;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Phase 2
/////////////////////////////////////////////////////////////////////////////////////////////

void Analyzer::indexSymbols(const IncludeGraph& graph, const std::vector<size_t>& order,
                            SymbolsDatabase& db)
{
    const auto& entries = graph.entries();
    for (size_t idx : order)
    {
        indexFile(entries[idx].sourceNode, db);
    }
}

void Analyzer::indexFile(const std::shared_ptr<SourceNode>& node, SymbolsDatabase& db)
{
    auto filePath = node->source->path;
    std::string nsPrefix;
    std::vector<std::string> usingNamespaces;
    walkChildren(node->children, nsPrefix, usingNamespaces, db, SymbolID::invalid(), filePath);
}

void Analyzer::walkChildren(const std::vector<std::shared_ptr<Node>>& children,
                            const std::string& nsPrefix,
                            const std::vector<std::string>& usingNamespaces, SymbolsDatabase& db,
                            SymbolID parent, const std::filesystem::path& filePath)
{
    // Copy usingNamespaces so it reverts on scope exit
    auto localUsing = usingNamespaces;

    for (const auto& child : children)
    {
        switch (child->kind)
        {
            case NodeKind::Namespace:
            {
                auto* ns = static_cast<NamespaceNode*>(child.get());
                std::string nsName = ns->isAnonymous ? "<anonymous>" : ns->name;

                // Handle nested namespaces like "a::b::c"
                // Split on "::" and create nested namespace symbols
                std::vector<std::string> parts;
                if (ns->isNested && !ns->isAnonymous)
                {
                    std::string tmp = nsName;
                    size_t pos = 0;
                    while ((pos = tmp.find("::")) != std::string::npos)
                    {
                        parts.push_back(tmp.substr(0, pos));
                        tmp = tmp.substr(pos + 2);
                    }
                    parts.push_back(tmp);
                }
                else
                {
                    parts.push_back(nsName);
                }

                std::string currentPrefix = nsPrefix;
                SymbolID currentParent = parent;
                for (const auto& part : parts)
                {
                    std::string qualName =
                        currentPrefix.empty() ? part : currentPrefix + "::" + part;

                    // Check if namespace already exists
                    auto* existing = db.findByQualifiedName(qualName);
                    SymbolID nsID;
                    if (existing && existing->symbolKind == SymbolKind::Namespace)
                    {
                        nsID = existing->id;
                    }
                    else
                    {
                        nsID = registerNode(child, SymbolKind::Namespace, part, qualName, "",
                                            AccessSpecifier::None, filePath, db, currentParent);
                    }

                    currentPrefix = qualName;
                    currentParent = nsID;
                }

                walkChildren(ns->children, currentPrefix, localUsing, db, currentParent, filePath);
                break;
            }

            case NodeKind::UsingNamespace:
            {
                auto* un = static_cast<UsingNamespaceNode*>(child.get());
                // Resolve relative to current namespace
                if (!nsPrefix.empty() && un->name.find("::") == std::string::npos)
                {
                    // Could be relative: try nsPrefix::name first
                    localUsing.push_back(nsPrefix + "::" + un->name);
                }
                localUsing.push_back(un->name);
                break;
            }

            case NodeKind::Class:
            {
                auto* cls = static_cast<ClassNode*>(child.get());
                if (cls->name.empty()) break;

                std::string qualName = nsPrefix.empty() ? cls->name : nsPrefix + "::" + cls->name;

                // Forward declaration detection: no members at all
                bool isForward = cls->memberVariables.empty() && cls->memberFunctions.empty() &&
                                 cls->staticMemberVariables.empty() &&
                                 cls->staticMemberFunctions.empty() && cls->constructors.empty() &&
                                 cls->destructors.empty() && cls->operators.empty() &&
                                 cls->nestedTypes.empty() && cls->baseClasses.empty();

                // Check for existing forward decl
                auto* existing = db.findByQualifiedName(qualName);
                if (existing && existing->isForwardDeclaration && !isForward)
                {
                    // Replace forward decl with full definition in-place
                    auto* mut = db.mutableFindByID(existing->id);
                    mut->isForwardDeclaration = false;
                    mut->node = child;
                    mut->location = {filePath, child->startLine, child->startColumn, child->endLine,
                                     child->endColumn};
                    walkClassMembers(*cls, qualName, localUsing, db, existing->id, filePath);
                    break;
                }

                if (existing && !existing->isForwardDeclaration)
                {
                    // Redefinition: keep first
                    break;
                }

                SymbolID clsID = registerNode(child, SymbolKind::Class, cls->name, qualName, "",
                                              AccessSpecifier::None, filePath, db, parent);
                if (isForward)
                {
                    auto* sym = db.mutableFindByID(clsID);
                    sym->isForwardDeclaration = true;
                }
                else
                {
                    walkClassMembers(*cls, qualName, localUsing, db, clsID, filePath);
                }
                break;
            }

            case NodeKind::Struct:
            {
                auto* s = static_cast<StructNode*>(child.get());
                if (s->name.empty()) break;

                std::string qualName = nsPrefix.empty() ? s->name : nsPrefix + "::" + s->name;

                bool isForward = s->memberVariables.empty() && s->memberFunctions.empty() &&
                                 s->staticMemberVariables.empty() &&
                                 s->staticMemberFunctions.empty() && s->constructors.empty() &&
                                 s->destructors.empty() && s->operators.empty() &&
                                 s->nestedTypes.empty() && s->baseClasses.empty();

                auto* existing = db.findByQualifiedName(qualName);
                if (existing && existing->isForwardDeclaration && !isForward)
                {
                    auto* mut = db.mutableFindByID(existing->id);
                    mut->isForwardDeclaration = false;
                    mut->node = child;
                    mut->location = {filePath, child->startLine, child->startColumn, child->endLine,
                                     child->endColumn};
                    walkStructMembers(*s, qualName, localUsing, db, existing->id, filePath);
                    break;
                }

                if (existing && !existing->isForwardDeclaration) break;

                SymbolID sID = registerNode(child, SymbolKind::Struct, s->name, qualName, "",
                                            AccessSpecifier::None, filePath, db, parent);
                if (isForward)
                {
                    auto* sym = db.mutableFindByID(sID);
                    sym->isForwardDeclaration = true;
                }
                else
                {
                    walkStructMembers(*s, qualName, localUsing, db, sID, filePath);
                }
                break;
            }

            case NodeKind::Union:
            {
                auto* u = static_cast<UnionNode*>(child.get());
                std::string uName = u->isAnonymous ? "<anonymous>" : u->name;
                if (uName.empty()) uName = "<anonymous>";

                std::string qualName = nsPrefix.empty() ? uName : nsPrefix + "::" + uName;

                SymbolID uID = registerNode(child, SymbolKind::Union, uName, qualName, "",
                                            AccessSpecifier::None, filePath, db, parent);
                walkUnionMembers(*u, qualName, localUsing, db, uID, filePath);
                break;
            }

            case NodeKind::EnumSpecifier:
            {
                auto* e = static_cast<EnumNode*>(child.get());
                std::string eName = e->name.empty() ? "<anonymous>" : e->name;
                std::string qualName = nsPrefix.empty() ? eName : nsPrefix + "::" + eName;

                SymbolID enumID = registerNode(child, SymbolKind::Enum, eName, qualName, "",
                                               AccessSpecifier::None, filePath, db, parent);

                for (const auto& enumerator : e->enumerators)
                {
                    if (enumerator->kind != NodeKind::Enum) continue;
                    auto* ev = static_cast<EnumSpecifierNode*>(enumerator.get());
                    std::string evQual = qualName + "::" + ev->name;
                    registerNode(enumerator, SymbolKind::EnumValue, ev->name, evQual, "",
                                 AccessSpecifier::None, filePath, db, enumID);
                }
                break;
            }

            case NodeKind::Function:
            {
                auto* fn = static_cast<FunctionNode*>(child.get());
                std::string qualName = nsPrefix.empty() ? fn->name : nsPrefix + "::" + fn->name;
                std::string sig = buildSignature(fn->parameters);

                registerNode(child, SymbolKind::Function, fn->name, qualName, sig,
                             AccessSpecifier::None, filePath, db, parent);
                break;
            }

            case NodeKind::Constructor:
            {
                auto* ctor = static_cast<ConstructorNode*>(child.get());
                std::string qualName = nsPrefix.empty() ? ctor->name : nsPrefix + "::" + ctor->name;
                std::string sig = buildSignature(ctor->parameters);

                registerNode(child, SymbolKind::Constructor, ctor->name, qualName, sig,
                             AccessSpecifier::None, filePath, db, parent);
                break;
            }

            case NodeKind::Destructor:
            {
                auto* dtor = static_cast<DestructorNode*>(child.get());
                std::string qualName = nsPrefix.empty() ? dtor->name : nsPrefix + "::" + dtor->name;

                registerNode(child, SymbolKind::Destructor, dtor->name, qualName, "",
                             AccessSpecifier::None, filePath, db, parent);
                break;
            }

            case NodeKind::Operator:
            {
                auto* op = static_cast<OperatorNode*>(child.get());
                std::string opName = "operator" + op->operatorSymbol;
                std::string qualName = nsPrefix.empty() ? opName : nsPrefix + "::" + opName;
                std::string sig = buildSignature(op->parameters);

                registerNode(child, SymbolKind::Operator, opName, qualName, sig,
                             AccessSpecifier::None, filePath, db, parent);
                break;
            }

            case NodeKind::Variable:
            {
                auto* var = static_cast<VariableNode*>(child.get());
                std::string qualName = nsPrefix.empty() ? var->name : nsPrefix + "::" + var->name;

                registerNode(child, SymbolKind::Variable, var->name, qualName, "",
                             AccessSpecifier::None, filePath, db, parent);
                break;
            }

            case NodeKind::TypeAlias:
            {
                auto* ta = static_cast<TypeAliasNode*>(child.get());
                std::string qualName =
                    nsPrefix.empty() ? ta->aliasName : nsPrefix + "::" + ta->aliasName;

                registerNode(child, SymbolKind::TypeAlias, ta->aliasName, qualName, "",
                             AccessSpecifier::None, filePath, db, parent);
                break;
            }

            case NodeKind::Typedef:
            {
                auto* td = static_cast<TypedefNode*>(child.get());
                std::string qualName =
                    nsPrefix.empty() ? td->aliasName : nsPrefix + "::" + td->aliasName;

                registerNode(child, SymbolKind::TypeAlias, td->aliasName, qualName, "",
                             AccessSpecifier::None, filePath, db, parent);
                break;
            }

            case NodeKind::Concept:
            {
                auto* c = static_cast<ConceptNode*>(child.get());
                std::string qualName = nsPrefix.empty() ? c->name : nsPrefix + "::" + c->name;

                registerNode(child, SymbolKind::Concept, c->name, qualName, "",
                             AccessSpecifier::None, filePath, db, parent);
                break;
            }

            case NodeKind::ObjectLikeMacro:
            {
                auto* m = static_cast<ObjectLikeMacroNode*>(child.get());
                // Macros are always global scope
                registerNode(child, SymbolKind::ObjectLikeMacro, m->name, m->name, "",
                             AccessSpecifier::None, filePath, db, parent);
                break;
            }

            case NodeKind::FunctionLikeMacro:
            {
                auto* m = static_cast<FunctionLikeMacroNode*>(child.get());
                registerNode(child, SymbolKind::FunctionLikeMacro, m->name, m->name, "",
                             AccessSpecifier::None, filePath, db, parent);
                break;
            }

            default:
                break;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Struct/Class/Union member walkers
/////////////////////////////////////////////////////////////////////////////////////////////

void Analyzer::walkStructMembers(const StructNode& s, const std::string& qualPrefix,
                                 const std::vector<std::string>& usingNamespaces,
                                 SymbolsDatabase& db, SymbolID parent,
                                 const std::filesystem::path& filePath)
{
    // Helper to walk a member vector (no access specifier)
    auto walkMembers =
        [&](const std::vector<std::shared_ptr<Node>>& members, AccessSpecifier defaultAccess)
    {
        for (const auto& member : members)
        {
            switch (member->kind)
            {
                case NodeKind::Variable:
                {
                    auto* var = static_cast<VariableNode*>(member.get());
                    std::string qual = qualPrefix + "::" + var->name;
                    registerNode(member, SymbolKind::Variable, var->name, qual, "", defaultAccess,
                                 filePath, db, parent);
                    break;
                }
                case NodeKind::Function:
                {
                    auto* fn = static_cast<FunctionNode*>(member.get());
                    std::string qual = qualPrefix + "::" + fn->name;
                    std::string sig = buildSignature(fn->parameters);
                    registerNode(member, SymbolKind::Function, fn->name, qual, sig, defaultAccess,
                                 filePath, db, parent);
                    break;
                }
                case NodeKind::Constructor:
                {
                    auto* ctor = static_cast<ConstructorNode*>(member.get());
                    std::string qual = qualPrefix + "::" + ctor->name;
                    std::string sig = buildSignature(ctor->parameters);
                    registerNode(member, SymbolKind::Constructor, ctor->name, qual, sig,
                                 defaultAccess, filePath, db, parent);
                    break;
                }
                case NodeKind::Destructor:
                {
                    auto* dtor = static_cast<DestructorNode*>(member.get());
                    std::string qual = qualPrefix + "::" + dtor->name;
                    registerNode(member, SymbolKind::Destructor, dtor->name, qual, "",
                                 defaultAccess, filePath, db, parent);
                    break;
                }
                case NodeKind::Operator:
                {
                    auto* op = static_cast<OperatorNode*>(member.get());
                    std::string opName = "operator" + op->operatorSymbol;
                    std::string qual = qualPrefix + "::" + opName;
                    std::string sig = buildSignature(op->parameters);
                    registerNode(member, SymbolKind::Operator, opName, qual, sig, defaultAccess,
                                 filePath, db, parent);
                    break;
                }
                case NodeKind::Struct:
                case NodeKind::Class:
                case NodeKind::Union:
                case NodeKind::EnumSpecifier:
                {
                    // Nested types: recurse through walkChildren
                    std::vector<std::shared_ptr<Node>> vec = {member};
                    walkChildren(vec, qualPrefix, usingNamespaces, db, parent, filePath);
                    break;
                }
                default:
                    break;
            }
        }
    };

    // Struct default access is Public
    walkMembers(s.memberVariables, AccessSpecifier::Public);
    walkMembers(s.memberFunctions, AccessSpecifier::Public);
    walkMembers(s.staticMemberVariables, AccessSpecifier::Public);
    walkMembers(s.staticMemberFunctions, AccessSpecifier::Public);
    walkMembers(s.constructors, AccessSpecifier::Public);
    walkMembers(s.destructors, AccessSpecifier::Public);
    walkMembers(s.operators, AccessSpecifier::Public);
    walkMembers(s.nestedTypes, AccessSpecifier::Public);
}

void Analyzer::walkClassMembers(const ClassNode& cls, const std::string& qualPrefix,
                                const std::vector<std::string>& usingNamespaces,
                                SymbolsDatabase& db, SymbolID parent,
                                const std::filesystem::path& filePath)
{
    // Helper to walk a member vector with per-member access specifier
    auto walkMembers =
        [&](const std::vector<std::pair<AccessSpecifier, std::shared_ptr<Node>>>& members)
    {
        for (const auto& [access, member] : members)
        {
            switch (member->kind)
            {
                case NodeKind::Variable:
                {
                    auto* var = static_cast<VariableNode*>(member.get());
                    std::string qual = qualPrefix + "::" + var->name;
                    registerNode(member, SymbolKind::Variable, var->name, qual, "", access,
                                 filePath, db, parent);
                    break;
                }
                case NodeKind::Function:
                {
                    auto* fn = static_cast<FunctionNode*>(member.get());
                    std::string qual = qualPrefix + "::" + fn->name;
                    std::string sig = buildSignature(fn->parameters);
                    registerNode(member, SymbolKind::Function, fn->name, qual, sig, access,
                                 filePath, db, parent);
                    break;
                }
                case NodeKind::Constructor:
                {
                    auto* ctor = static_cast<ConstructorNode*>(member.get());
                    std::string qual = qualPrefix + "::" + ctor->name;
                    std::string sig = buildSignature(ctor->parameters);
                    registerNode(member, SymbolKind::Constructor, ctor->name, qual, sig, access,
                                 filePath, db, parent);
                    break;
                }
                case NodeKind::Destructor:
                {
                    auto* dtor = static_cast<DestructorNode*>(member.get());
                    std::string qual = qualPrefix + "::" + dtor->name;
                    registerNode(member, SymbolKind::Destructor, dtor->name, qual, "", access,
                                 filePath, db, parent);
                    break;
                }
                case NodeKind::Operator:
                {
                    auto* op = static_cast<OperatorNode*>(member.get());
                    std::string opName = "operator" + op->operatorSymbol;
                    std::string qual = qualPrefix + "::" + opName;
                    std::string sig = buildSignature(op->parameters);
                    registerNode(member, SymbolKind::Operator, opName, qual, sig, access, filePath,
                                 db, parent);
                    break;
                }
                case NodeKind::Struct:
                case NodeKind::Class:
                case NodeKind::Union:
                case NodeKind::EnumSpecifier:
                {
                    std::vector<std::shared_ptr<Node>> vec = {member};
                    walkChildren(vec, qualPrefix, usingNamespaces, db, parent, filePath);
                    break;
                }
                default:
                    break;
            }
        }
    };

    walkMembers(cls.memberVariables);
    walkMembers(cls.memberFunctions);
    walkMembers(cls.staticMemberVariables);
    walkMembers(cls.staticMemberFunctions);
    walkMembers(cls.constructors);
    walkMembers(cls.destructors);
    walkMembers(cls.operators);
    walkMembers(cls.nestedTypes);
}

void Analyzer::walkUnionMembers(const UnionNode& u, const std::string& qualPrefix,
                                const std::vector<std::string>& usingNamespaces,
                                SymbolsDatabase& db, SymbolID parent,
                                const std::filesystem::path& filePath)
{
    auto walkMembers = [&](const std::vector<std::shared_ptr<Node>>& members)
    {
        for (const auto& member : members)
        {
            switch (member->kind)
            {
                case NodeKind::Variable:
                {
                    auto* var = static_cast<VariableNode*>(member.get());
                    std::string qual = qualPrefix + "::" + var->name;
                    registerNode(member, SymbolKind::Variable, var->name, qual, "",
                                 AccessSpecifier::Public, filePath, db, parent);
                    break;
                }
                case NodeKind::Function:
                {
                    auto* fn = static_cast<FunctionNode*>(member.get());
                    std::string qual = qualPrefix + "::" + fn->name;
                    std::string sig = buildSignature(fn->parameters);
                    registerNode(member, SymbolKind::Function, fn->name, qual, sig,
                                 AccessSpecifier::Public, filePath, db, parent);
                    break;
                }
                case NodeKind::Constructor:
                {
                    auto* ctor = static_cast<ConstructorNode*>(member.get());
                    std::string qual = qualPrefix + "::" + ctor->name;
                    std::string sig = buildSignature(ctor->parameters);
                    registerNode(member, SymbolKind::Constructor, ctor->name, qual, sig,
                                 AccessSpecifier::Public, filePath, db, parent);
                    break;
                }
                case NodeKind::Destructor:
                {
                    auto* dtor = static_cast<DestructorNode*>(member.get());
                    std::string qual = qualPrefix + "::" + dtor->name;
                    registerNode(member, SymbolKind::Destructor, dtor->name, qual, "",
                                 AccessSpecifier::Public, filePath, db, parent);
                    break;
                }
                case NodeKind::Operator:
                {
                    auto* op = static_cast<OperatorNode*>(member.get());
                    std::string opName = "operator" + op->operatorSymbol;
                    std::string qual = qualPrefix + "::" + opName;
                    std::string sig = buildSignature(op->parameters);
                    registerNode(member, SymbolKind::Operator, opName, qual, sig,
                                 AccessSpecifier::Public, filePath, db, parent);
                    break;
                }
                case NodeKind::Struct:
                case NodeKind::Class:
                case NodeKind::Union:
                case NodeKind::EnumSpecifier:
                {
                    std::vector<std::shared_ptr<Node>> vec = {member};
                    walkChildren(vec, qualPrefix, usingNamespaces, db, parent, filePath);
                    break;
                }
                default:
                    break;
            }
        }
    };

    walkMembers(u.memberVariables);
    walkMembers(u.memberFunctions);
    walkMembers(u.staticMemberVariables);
    walkMembers(u.staticMemberFunctions);
    walkMembers(u.constructors);
    walkMembers(u.destructors);
    walkMembers(u.operators);
    walkMembers(u.nestedTypes);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Phase 3
/////////////////////////////////////////////////////////////////////////////////////////////

void Analyzer::linkCrossReferences(SymbolsDatabase& db)
{
    auto allSyms = db.allSymbols();

    // Collect namespace prefix info for each symbol
    // Map from symbol ID to its namespace context
    struct SymbolContext
    {
        std::string nsPrefix;
        std::vector<std::string> usingNs;
    };

    // Build namespace prefix for each symbol by walking parent chain
    auto getNsPrefix = [&](const Symbol& sym) -> std::string
    {
        std::string ns;
        // qualifiedName minus the last component gives us the namespace
        auto pos = sym.qualifiedName.rfind("::");
        if (pos != std::string::npos) ns = sym.qualifiedName.substr(0, pos);
        return ns;
    };

    for (const auto* sym : allSyms)
    {
        auto* mutSym = db.mutableFindByID(sym->id);
        if (!mutSym) continue;
        if (!mutSym->node) continue;

        std::string currentNs = getNsPrefix(*sym);

        // 1. Base classes (for Class and Struct nodes)
        if (sym->symbolKind == SymbolKind::Class && sym->node->kind == NodeKind::Class)
        {
            auto* cls = static_cast<const ClassNode*>(sym->node.get());
            for (const auto& [access, baseName] : cls->baseClasses)
            {
                SymbolID baseID = resolveTypeName(baseName, currentNs, {}, db);
                if (baseID)
                {
                    mutSym->baseClasses.push_back(baseID);
                    if (auto* baseSym = db.mutableFindByID(baseID))
                    {
                        baseSym->derivedClasses.push_back(sym->id);
                    }
                }
            }
        }
        else if (sym->symbolKind == SymbolKind::Struct && sym->node->kind == NodeKind::Struct)
        {
            auto* s = static_cast<const StructNode*>(sym->node.get());
            for (const auto& [access, baseName] : s->baseClasses)
            {
                SymbolID baseID = resolveTypeName(baseName, currentNs, {}, db);
                if (baseID)
                {
                    mutSym->baseClasses.push_back(baseID);
                    if (auto* baseSym = db.mutableFindByID(baseID))
                    {
                        baseSym->derivedClasses.push_back(sym->id);
                    }
                }
            }
        }

        // 2. Type references from functions, variables, type aliases
        std::vector<std::string> typeNames;
        collectTypeRefs(*sym, typeNames);

        // Deduplicate
        std::unordered_set<std::string> seen;
        for (const auto& tn : typeNames)
        {
            if (tn.empty() || !seen.insert(tn).second) continue;

            SymbolID refID = resolveTypeName(tn, currentNs, {}, db);
            if (refID && refID != sym->id)
            {
                mutSym->referencedTypes.push_back(refID);
                if (auto* refSym = db.mutableFindByID(refID))
                {
                    refSym->referencedBy.push_back(sym->id);
                }
            }
        }
    }
}

void Analyzer::collectTypeRefs(const Symbol& sym, std::vector<std::string>& outTypes) const
{
    if (!sym.node) return;

    switch (sym.node->kind)
    {
        case NodeKind::Function:
        {
            auto* fn = static_cast<const FunctionNode*>(sym.node.get());
            collectTypeSignatureRefs(fn->returnSignature, outTypes);
            for (const auto& param : fn->parameters)
            {
                collectTypeSignatureRefs(param.typeSignature, outTypes);
            }
            break;
        }
        case NodeKind::Constructor:
        {
            auto* ctor = static_cast<const ConstructorNode*>(sym.node.get());
            for (const auto& param : ctor->parameters)
            {
                collectTypeSignatureRefs(param.typeSignature, outTypes);
            }
            break;
        }
        case NodeKind::Operator:
        {
            auto* op = static_cast<const OperatorNode*>(sym.node.get());
            collectTypeSignatureRefs(op->returnSignature, outTypes);
            for (const auto& param : op->parameters)
            {
                collectTypeSignatureRefs(param.typeSignature, outTypes);
            }
            break;
        }
        case NodeKind::Variable:
        {
            auto* var = static_cast<const VariableNode*>(sym.node.get());
            collectTypeSignatureRefs(var->typeSignature, outTypes);
            break;
        }
        case NodeKind::TypeAlias:
        {
            auto* ta = static_cast<const TypeAliasNode*>(sym.node.get());
            // targetType is a plain string, add it directly
            if (!ta->targetType.empty()) outTypes.push_back(ta->targetType);
            break;
        }
        case NodeKind::Typedef:
        {
            auto* td = static_cast<const TypedefNode*>(sym.node.get());
            if (!td->targetType.empty()) outTypes.push_back(td->targetType);
            break;
        }
        default:
            break;
    }
}

void Analyzer::collectTypeSignatureRefs(const TypeSignature& ts,
                                        std::vector<std::string>& out) const
{
    if (!ts.baseType.empty())
    {
        out.push_back(ts.baseType);
    }
    for (const auto& arg : ts.templateArgs)
    {
        if (!arg.typeSignature.baseType.empty())
        {
            collectTypeSignatureRefs(arg.typeSignature, out);
        }
    }
}

SymbolID Analyzer::resolveTypeName(const std::string& typeName, const std::string& currentNs,
                                   const std::vector<std::string>& usingNs,
                                   const SymbolsDatabase& db) const
{
    if (typeName.empty()) return SymbolID::invalid();

    // Strip leading :: for absolute qualification
    std::string name = typeName;
    if (name.size() >= 2 && name[0] == ':' && name[1] == ':')
    {
        name = name.substr(2);
    }

    // 1. If contains "::" → qualified lookup
    if (name.find("::") != std::string::npos)
    {
        auto* sym = db.findByQualifiedName(name);
        if (sym) return sym->id;
    }

    // 2. Walk up current namespace: currentNs::T, parent::T, ..., T
    if (!currentNs.empty())
    {
        std::string ns = currentNs;
        while (true)
        {
            std::string candidate = ns + "::" + name;
            auto* sym = db.findByQualifiedName(candidate);
            if (sym) return sym->id;

            auto pos = ns.rfind("::");
            if (pos == std::string::npos) break;
            ns = ns.substr(0, pos);
        }
    }

    // 3. Try each using-namespace
    for (const auto& uns : usingNs)
    {
        std::string candidate = uns + "::" + name;
        auto* sym = db.findByQualifiedName(candidate);
        if (sym) return sym->id;
    }

    // 4. Try global
    auto* sym = db.findByQualifiedName(name);
    if (sym) return sym->id;

    return SymbolID::invalid();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Helpers
/////////////////////////////////////////////////////////////////////////////////////////////

SymbolID Analyzer::registerNode(const std::shared_ptr<Node>& node, SymbolKind kind,
                                const std::string& name, const std::string& qualifiedName,
                                const std::string& signature, AccessSpecifier access,
                                const std::filesystem::path& filePath, SymbolsDatabase& db,
                                SymbolID parent)
{
    Symbol sym;
    sym.symbolKind = kind;
    sym.name = name;
    sym.qualifiedName = qualifiedName;
    sym.signature = signature;
    sym.access = access;
    sym.node = node;
    sym.location = {filePath, node->startLine, node->startColumn, node->endLine, node->endColumn};
    sym.parentSymbol = parent;

    SymbolID id = db.addSymbol(std::move(sym));

    // Update parent's child list
    if (parent)
    {
        if (auto* parentSym = db.mutableFindByID(parent))
        {
            parentSym->childSymbols.push_back(id);
        }
    }

    return id;
}

std::string Analyzer::buildSignature(const std::vector<FunctionParameter>& params) const
{
    std::string sig = "(";
    for (size_t i = 0; i < params.size(); ++i)
    {
        if (i > 0) sig += ", ";
        sig += params[i].typeSignature.toString();
    }
    sig += ")";
    return sig;
}

} // namespace codex
