#include "mdx_generator.hpp"
#include "comment_parser.hpp"
#include "mdx_formatter.hpp"

#include <iostream>
#include <sstream>

namespace docsgen
{

std::vector<MDXFile> MDXGenerator::generate(const codex::AnalysisResult& result)
{
    m_db = &result.database;
    m_linker.buildIndex(*m_db);

    std::vector<MDXFile> files;

    for (const codex::Symbol* sym : m_db->allSymbols())
    {
        if (!sym || !isPublicAPI(*sym)) continue;

        try
        {
            MDXFile file;
            switch (sym->symbolKind)
            {
                case codex::SymbolKind::Class:
                    file = generateClassDoc(*sym);
                    break;
                case codex::SymbolKind::Struct:
                    file = generateStructDoc(*sym);
                    break;
                case codex::SymbolKind::Enum:
                    file = generateEnumDoc(*sym);
                    break;
                case codex::SymbolKind::Function:
                    file = generateFunctionDoc(*sym);
                    break;
                case codex::SymbolKind::TypeAlias:
                    file = generateTypeAliasDoc(*sym);
                    break;
                case codex::SymbolKind::Concept:
                    file = generateConceptDoc(*sym);
                    break;
                case codex::SymbolKind::Operator:
                    file = generateOperatorDoc(*sym);
                    break;
                default:
                    continue;
            }

            if (!file.filename.empty())
            {
                files.push_back(std::move(file));
                if (m_verbose) std::cout << "  Generated: " << file.filename << "\n";
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error generating doc for " << sym->qualifiedName << ": " << e.what()
                      << "\n";
        }
    }

    return files;
}

bool MDXGenerator::isPublicAPI(const codex::Symbol& sym) const
{
    if (sym.isForwardDeclaration) return false;

    if (sym.access == codex::AccessSpecifier::Private ||
        sym.access == codex::AccessSpecifier::Protected)
        return false;

    switch (sym.symbolKind)
    {
        case codex::SymbolKind::Class:
        case codex::SymbolKind::Struct:
        case codex::SymbolKind::Enum:
        case codex::SymbolKind::Function:
        case codex::SymbolKind::TypeAlias:
        case codex::SymbolKind::Concept:
        case codex::SymbolKind::Operator:
            return true;
        default:
            return false;
    }
}

std::string MDXGenerator::getComment(const std::shared_ptr<codex::Node>& node) const
{
    if (!node || !node->comment) return "";
    auto* comment = dynamic_cast<codex::CommentNode*>(node->comment.get());
    return comment ? comment->text : "";
}

ParsedComment MDXGenerator::parseNodeComment(const std::shared_ptr<codex::Node>& node) const
{
    std::string raw = getComment(node);
    if (raw.empty()) return {};
    return CommentParser::parse(raw);
}

std::string MDXGenerator::formatSeeAlsoFromComment(const ParsedComment& parsed) const
{
    std::string result;
    for (const auto& entry : parsed.seeAlso)
    {
        // URL
        if (entry.starts_with("http://") || entry.starts_with("https://"))
        {
            result += "- [" + entry + "](" + entry + ")\n";
            continue;
        }

        // Symbol lookup
        std::string filename = m_linker.findFilename(entry);
        if (!filename.empty())
        {
            result += "- [" + entry + "](/saturn/api_reference/" + filename + ")\n";
            continue;
        }

        // Plain text
        result += "- " + entry + "\n";
    }
    return result;
}

MDXFile MDXGenerator::generateClassDoc(const codex::Symbol& sym)
{
    auto* classNode = dynamic_cast<codex::ClassNode*>(sym.node.get());
    if (!classNode) return {};

    ParsedComment parsed = parseNodeComment(sym.node);

    MDXFile file;
    file.filename = CrossLinker::qualifiedToFilename(sym.qualifiedName);
    file.title = sym.name;
    file.description = parsed.brief;

    std::ostringstream oss;

    std::string desc = !parsed.description.empty() ? parsed.description : parsed.brief;
    if (!desc.empty()) oss << desc << "\n\n";

    // Source location
    oss << "*Defined in `" << MDXFormatter::formatSourceLocation(sym.location) << "`*\n\n";

    // Inheritance
    oss << generateInheritanceSection(sym);

    // Template
    oss << generateTemplateSection(classNode->templateDecl, parsed.tparams);

    // Constructors (public only)
    oss << generateConstructorsSection(classNode->constructors);

    // Destructor
    oss << generateDestructorsSection(classNode->destructors);

    // Public Methods
    oss << generateMethodsSection(classNode->memberFunctions);
    oss << generateMethodsSection(classNode->staticMemberFunctions);

    // Operators
    oss << generateOperatorsSection(classNode->operators);

    // Public Member Variables
    oss << generateMemberVarsSection(classNode->memberVariables);
    oss << generateMemberVarsSection(classNode->staticMemberVariables);

    // Notes
    oss << MDXFormatter::formatNotesSection(parsed.notes);

    // See Also
    oss << generateSeeAlsoSection(sym, parsed);

    file.content = oss.str();
    return file;
}

MDXFile MDXGenerator::generateStructDoc(const codex::Symbol& sym)
{
    auto* structNode = dynamic_cast<codex::StructNode*>(sym.node.get());
    if (!structNode) return {};

    ParsedComment parsed = parseNodeComment(sym.node);

    MDXFile file;
    file.filename = CrossLinker::qualifiedToFilename(sym.qualifiedName);
    file.title = sym.name;
    file.description = parsed.brief;

    std::ostringstream oss;

    std::string desc = !parsed.description.empty() ? parsed.description : parsed.brief;
    if (!desc.empty()) oss << desc << "\n\n";

    // Source location
    oss << "*Defined in `" << MDXFormatter::formatSourceLocation(sym.location) << "`*\n\n";

    // Inheritance
    oss << generateInheritanceSection(sym);

    // Template
    oss << generateTemplateSection(structNode->templateDecl, parsed.tparams);

    // Constructors
    oss << generateStructConstructorsSection(structNode->constructors);

    // Destructor
    oss << generateStructDestructorsSection(structNode->destructors);

    // Methods
    oss << generateStructMethodsSection(structNode->memberFunctions);
    oss << generateStructMethodsSection(structNode->staticMemberFunctions);

    // Operators
    oss << generateStructOperatorsSection(structNode->operators);

    // Member Variables
    oss << generateStructMemberVarsSection(structNode->memberVariables);
    oss << generateStructMemberVarsSection(structNode->staticMemberVariables);

    // Notes
    oss << MDXFormatter::formatNotesSection(parsed.notes);

    // See Also
    oss << generateSeeAlsoSection(sym, parsed);

    file.content = oss.str();
    return file;
}

MDXFile MDXGenerator::generateEnumDoc(const codex::Symbol& sym)
{
    auto* enumNode = dynamic_cast<codex::EnumNode*>(sym.node.get());
    if (!enumNode) return {};

    ParsedComment parsed = parseNodeComment(sym.node);

    MDXFile file;
    file.filename = CrossLinker::qualifiedToFilename(sym.qualifiedName);
    file.title = sym.name;
    file.description = parsed.brief;

    std::ostringstream oss;

    std::string desc = !parsed.description.empty() ? parsed.description : parsed.brief;
    if (!desc.empty()) oss << desc << "\n\n";

    // Source location
    oss << "*Defined in `" << MDXFormatter::formatSourceLocation(sym.location) << "`*\n\n";

    // Definition
    oss << "## Definition\n\n";
    oss << "```cpp\n";
    oss << MDXFormatter::formatEnumSignature(*enumNode) << " {\n";
    for (const auto& enumerator : enumNode->enumerators)
    {
        auto* spec = dynamic_cast<codex::EnumSpecifierNode*>(enumerator.get());
        if (!spec) continue;
        oss << "    " << spec->name;
        if (!spec->value.empty()) oss << " = " << spec->value;
        oss << ",\n";
    }
    oss << "};\n";
    oss << "```\n\n";

    // Values
    if (!enumNode->enumerators.empty())
    {
        oss << "## Values\n\n";
        oss << MDXFormatter::formatEnumValuesTable(enumNode->enumerators);
        oss << "\n";
    }

    // Notes
    oss << MDXFormatter::formatNotesSection(parsed.notes);

    // See Also
    oss << generateSeeAlsoSection(sym, parsed);

    file.content = oss.str();
    return file;
}

MDXFile MDXGenerator::generateFunctionDoc(const codex::Symbol& sym)
{
    auto* fnNode = dynamic_cast<codex::FunctionNode*>(sym.node.get());
    if (!fnNode) return {};

    ParsedComment parsed = parseNodeComment(sym.node);

    MDXFile file;
    file.filename = CrossLinker::qualifiedToFilename(sym.qualifiedName);
    file.title = sym.name;
    file.description = parsed.brief;

    std::ostringstream oss;

    std::string desc = !parsed.description.empty() ? parsed.description : parsed.brief;
    if (!desc.empty()) oss << desc << "\n\n";

    // Source location
    oss << "*Defined in `" << MDXFormatter::formatSourceLocation(sym.location) << "`*\n\n";

    // Signature
    oss << "## Signature\n\n";
    oss << "```cpp\n";
    oss << MDXFormatter::formatFunctionSignature(*fnNode) << ";\n";
    oss << "```\n\n";

    // Template
    oss << generateTemplateSection(fnNode->templateDecl, parsed.tparams);

    // Parameters
    if (!fnNode->parameters.empty())
    {
        oss << "## Parameters\n\n";
        oss << MDXFormatter::formatParametersTable(fnNode->parameters, m_linker, parsed.params);
        oss << "\n";
    }

    // Returns
    if (!fnNode->returnSignature.baseType.empty() && fnNode->returnSignature.baseType != "void")
    {
        oss << "## Returns\n\n";
        oss << MDXFormatter::formatReturnWithDescription(
                   m_linker.linkifyTypeSignature(fnNode->returnSignature), parsed.returns)
            << "\n\n";
    }

    // Notes
    oss << MDXFormatter::formatNotesSection(parsed.notes);

    // See Also
    oss << generateSeeAlsoSection(sym, parsed);

    file.content = oss.str();
    return file;
}

MDXFile MDXGenerator::generateTypeAliasDoc(const codex::Symbol& sym)
{
    auto* aliasNode = dynamic_cast<codex::TypeAliasNode*>(sym.node.get());
    if (!aliasNode) return {};

    ParsedComment parsed = parseNodeComment(sym.node);

    MDXFile file;
    file.filename = CrossLinker::qualifiedToFilename(sym.qualifiedName);
    file.title = sym.name;
    file.description = parsed.brief;

    std::ostringstream oss;

    std::string desc = !parsed.description.empty() ? parsed.description : parsed.brief;
    if (!desc.empty()) oss << desc << "\n\n";

    // Source location
    oss << "*Defined in `" << MDXFormatter::formatSourceLocation(sym.location) << "`*\n\n";

    // Definition
    oss << "## Definition\n\n";
    oss << "```cpp\n";
    oss << MDXFormatter::formatTypeAliasSignature(*aliasNode) << ";\n";
    oss << "```\n\n";

    // Template
    oss << generateTemplateSection(aliasNode->templateDecl, parsed.tparams);

    // Aliased Type
    oss << "## Aliased Type\n\n";
    oss << m_linker.linkify(aliasNode->targetType) << "\n\n";

    // Notes
    oss << MDXFormatter::formatNotesSection(parsed.notes);

    // See Also
    oss << generateSeeAlsoSection(sym, parsed);

    file.content = oss.str();
    return file;
}

MDXFile MDXGenerator::generateConceptDoc(const codex::Symbol& sym)
{
    auto* conceptNode = dynamic_cast<codex::ConceptNode*>(sym.node.get());
    if (!conceptNode) return {};

    ParsedComment parsed = parseNodeComment(sym.node);

    MDXFile file;
    file.filename = CrossLinker::qualifiedToFilename(sym.qualifiedName);
    file.title = sym.name;
    file.description = parsed.brief;

    std::ostringstream oss;

    std::string desc = !parsed.description.empty() ? parsed.description : parsed.brief;
    if (!desc.empty()) oss << desc << "\n\n";

    // Source location
    oss << "*Defined in `" << MDXFormatter::formatSourceLocation(sym.location) << "`*\n\n";

    // Definition
    oss << "## Definition\n\n";
    oss << "```cpp\n";
    oss << MDXFormatter::formatConceptSignature(*conceptNode) << ";\n";
    oss << "```\n\n";

    // Template
    oss << generateTemplateSection(conceptNode->templateDecl, parsed.tparams);

    // Constraint
    oss << "## Constraint\n\n";
    oss << "```cpp\n";
    oss << conceptNode->constraint << "\n";
    oss << "```\n\n";

    // Notes
    oss << MDXFormatter::formatNotesSection(parsed.notes);

    // See Also
    oss << generateSeeAlsoSection(sym, parsed);

    file.content = oss.str();
    return file;
}

MDXFile MDXGenerator::generateOperatorDoc(const codex::Symbol& sym)
{
    auto* opNode = dynamic_cast<codex::OperatorNode*>(sym.node.get());
    if (!opNode) return {};

    ParsedComment parsed = parseNodeComment(sym.node);

    MDXFile file;
    file.filename = CrossLinker::qualifiedToFilename(sym.qualifiedName);
    file.title = "operator" + opNode->operatorSymbol;
    file.description = parsed.brief;

    std::ostringstream oss;

    std::string desc = !parsed.description.empty() ? parsed.description : parsed.brief;
    if (!desc.empty()) oss << desc << "\n\n";

    // Source location
    oss << "*Defined in `" << MDXFormatter::formatSourceLocation(sym.location) << "`*\n\n";

    // Signature
    oss << "## Signature\n\n";
    oss << "```cpp\n";
    oss << MDXFormatter::formatOperatorSignature(*opNode) << ";\n";
    oss << "```\n\n";

    // Template
    oss << generateTemplateSection(opNode->templateDecl, parsed.tparams);

    // Parameters
    if (!opNode->parameters.empty())
    {
        oss << "## Parameters\n\n";
        oss << MDXFormatter::formatParametersTable(opNode->parameters, m_linker, parsed.params);
        oss << "\n";
    }

    // Returns
    if (!opNode->returnSignature.baseType.empty() && opNode->returnSignature.baseType != "void")
    {
        oss << "## Returns\n\n";
        oss << MDXFormatter::formatReturnWithDescription(
                   m_linker.linkifyTypeSignature(opNode->returnSignature), parsed.returns)
            << "\n\n";
    }

    // Notes
    oss << MDXFormatter::formatNotesSection(parsed.notes);

    // See Also
    oss << generateSeeAlsoSection(sym, parsed);

    file.content = oss.str();
    return file;
}

std::string MDXGenerator::generateInheritanceSection(const codex::Symbol& sym)
{
    std::ostringstream oss;

    if (!sym.baseClasses.empty())
    {
        oss << "## Inheritance\n\n";
        oss << "Inherits from:\n";
        for (codex::SymbolID baseId : sym.baseClasses)
        {
            const codex::Symbol* baseSym = m_db->findByID(baseId);
            if (baseSym)
            {
                std::string filename = CrossLinker::qualifiedToFilename(baseSym->qualifiedName);
                oss << "- [" << baseSym->name << "](/saturn/api_reference/" << filename << ")\n";
            }
        }
        oss << "\n";
    }

    if (!sym.derivedClasses.empty())
    {
        oss << "### Derived Classes\n\n";
        for (codex::SymbolID derivedId : sym.derivedClasses)
        {
            const codex::Symbol* derivedSym = m_db->findByID(derivedId);
            if (derivedSym)
            {
                std::string filename = CrossLinker::qualifiedToFilename(derivedSym->qualifiedName);
                oss << "- [" << derivedSym->name << "](/saturn/api_reference/" << filename << ")\n";
            }
        }
        oss << "\n";
    }

    return oss.str();
}

std::string MDXGenerator::generateTemplateSection(
    const std::shared_ptr<codex::Node>& templateDecl,
    const std::unordered_map<std::string, std::string>& tparamDescs)
{
    if (!templateDecl) return "";

    auto* tmpl = dynamic_cast<codex::TemplateNode*>(templateDecl.get());
    if (!tmpl || tmpl->parameters.empty()) return "";

    std::ostringstream oss;
    oss << "## Template Parameters\n\n";
    if (tparamDescs.empty())
        oss << MDXFormatter::formatTemplateParamsTable(tmpl->parameters);
    else
        oss << MDXFormatter::formatTemplateParamsTable(tmpl->parameters, tparamDescs);
    oss << "\n";

    return oss.str();
}

std::string MDXGenerator::generateConstructorsSection(
    const std::vector<std::pair<codex::AccessSpecifier, std::shared_ptr<codex::Node>>>& ctors)
{
    // Filter public only
    std::vector<std::shared_ptr<codex::Node>> publicCtors;
    for (const auto& [access, node] : ctors)
    {
        if (access == codex::AccessSpecifier::Public) publicCtors.push_back(node);
    }

    if (publicCtors.empty()) return "";

    std::ostringstream oss;
    oss << "## Constructors\n\n";

    for (const auto& node : publicCtors)
    {
        auto* ctor = dynamic_cast<codex::ConstructorNode*>(node.get());
        if (!ctor) continue;

        ParsedComment parsed = parseNodeComment(node);
        std::string desc = !parsed.description.empty() ? parsed.description : parsed.brief;
        if (!desc.empty()) oss << desc << "\n\n";

        oss << "```cpp\n";
        oss << MDXFormatter::formatConstructorSignature(*ctor) << ";\n";
        oss << "```\n\n";

        if (!ctor->parameters.empty())
        {
            oss << "**Parameters:**\n\n";
            oss << MDXFormatter::formatParametersTable(ctor->parameters, m_linker, parsed.params);
            oss << "\n";
        }

        oss << MDXFormatter::formatNotesSection(parsed.notes);

        std::string seeAlso = formatSeeAlsoFromComment(parsed);
        if (!seeAlso.empty()) oss << "**See also:**\n\n" << seeAlso << "\n";
    }

    return oss.str();
}

std::string MDXGenerator::generateDestructorsSection(
    const std::vector<std::pair<codex::AccessSpecifier, std::shared_ptr<codex::Node>>>& dtors)
{
    // Filter public only
    std::vector<std::shared_ptr<codex::Node>> publicDtors;
    for (const auto& [access, node] : dtors)
    {
        if (access == codex::AccessSpecifier::Public) publicDtors.push_back(node);
    }

    if (publicDtors.empty()) return "";

    std::ostringstream oss;
    oss << "## Destructor\n\n";

    for (const auto& node : publicDtors)
    {
        auto* dtor = dynamic_cast<codex::DestructorNode*>(node.get());
        if (!dtor) continue;

        ParsedComment parsed = parseNodeComment(node);
        std::string desc = !parsed.description.empty() ? parsed.description : parsed.brief;
        if (!desc.empty()) oss << desc << "\n\n";

        oss << "```cpp\n";
        oss << MDXFormatter::formatDestructorSignature(*dtor) << ";\n";
        oss << "```\n\n";
    }

    return oss.str();
}

std::string MDXGenerator::generateMethodsSection(
    const std::vector<std::pair<codex::AccessSpecifier, std::shared_ptr<codex::Node>>>& methods)
{
    // Filter public only
    std::vector<std::shared_ptr<codex::Node>> publicMethods;
    for (const auto& [access, node] : methods)
    {
        if (access == codex::AccessSpecifier::Public) publicMethods.push_back(node);
    }

    if (publicMethods.empty()) return "";

    std::ostringstream oss;
    oss << "## Public Methods\n\n";

    for (const auto& node : publicMethods)
    {
        auto* fn = dynamic_cast<codex::FunctionNode*>(node.get());
        if (!fn) continue;

        oss << "### " << fn->name << "\n\n";

        ParsedComment parsed = parseNodeComment(node);
        std::string desc = !parsed.description.empty() ? parsed.description : parsed.brief;
        if (!desc.empty()) oss << desc << "\n\n";

        oss << "```cpp\n";
        oss << MDXFormatter::formatFunctionSignature(*fn) << ";\n";
        oss << "```\n\n";

        if (!fn->parameters.empty())
        {
            oss << "**Parameters:**\n\n";
            oss << MDXFormatter::formatParametersTable(fn->parameters, m_linker, parsed.params);
            oss << "\n";
        }

        if (!fn->returnSignature.baseType.empty() && fn->returnSignature.baseType != "void")
        {
            oss << "**Returns:** "
                << MDXFormatter::formatReturnWithDescription(
                       m_linker.linkifyTypeSignature(fn->returnSignature), parsed.returns)
                << "\n\n";
        }

        oss << MDXFormatter::formatNotesSection(parsed.notes);

        std::string seeAlso = formatSeeAlsoFromComment(parsed);
        if (!seeAlso.empty()) oss << "**See also:**\n\n" << seeAlso << "\n";
    }

    return oss.str();
}

std::string MDXGenerator::generateOperatorsSection(
    const std::vector<std::pair<codex::AccessSpecifier, std::shared_ptr<codex::Node>>>& ops)
{
    // Filter public only
    std::vector<std::shared_ptr<codex::Node>> publicOps;
    for (const auto& [access, node] : ops)
    {
        if (access == codex::AccessSpecifier::Public) publicOps.push_back(node);
    }

    if (publicOps.empty()) return "";

    std::ostringstream oss;
    oss << "## Operators\n\n";

    for (const auto& node : publicOps)
    {
        auto* op = dynamic_cast<codex::OperatorNode*>(node.get());
        if (!op) continue;

        oss << "### operator" << op->operatorSymbol << "\n\n";

        ParsedComment parsed = parseNodeComment(node);
        std::string desc = !parsed.description.empty() ? parsed.description : parsed.brief;
        if (!desc.empty()) oss << desc << "\n\n";

        oss << "```cpp\n";
        oss << MDXFormatter::formatOperatorSignature(*op) << ";\n";
        oss << "```\n\n";

        if (!op->parameters.empty())
        {
            oss << "**Parameters:**\n\n";
            oss << MDXFormatter::formatParametersTable(op->parameters, m_linker, parsed.params);
            oss << "\n";
        }

        if (!op->returnSignature.baseType.empty() && op->returnSignature.baseType != "void")
        {
            oss << "**Returns:** "
                << MDXFormatter::formatReturnWithDescription(
                       m_linker.linkifyTypeSignature(op->returnSignature), parsed.returns)
                << "\n\n";
        }

        oss << MDXFormatter::formatNotesSection(parsed.notes);

        std::string seeAlso = formatSeeAlsoFromComment(parsed);
        if (!seeAlso.empty()) oss << "**See also:**\n\n" << seeAlso << "\n";
    }

    return oss.str();
}

std::string MDXGenerator::generateMemberVarsSection(
    const std::vector<std::pair<codex::AccessSpecifier, std::shared_ptr<codex::Node>>>& vars)
{
    // Filter public only
    std::vector<std::shared_ptr<codex::Node>> publicVars;
    for (const auto& [access, node] : vars)
    {
        if (access == codex::AccessSpecifier::Public) publicVars.push_back(node);
    }

    if (publicVars.empty()) return "";

    std::ostringstream oss;
    oss << "## Public Members\n\n";

    oss << "| Name | Type | Description |\n";
    oss << "|------|------|-------------|\n";

    for (const auto& node : publicVars)
    {
        auto* var = dynamic_cast<codex::VariableNode*>(node.get());
        if (!var) continue;

        oss << "| `" << var->name << "` | ";
        oss << m_linker.linkifyTypeSignature(var->typeSignature) << " | ";

        ParsedComment parsed = parseNodeComment(node);
        oss << parsed.brief << " |\n";
    }

    oss << "\n";
    return oss.str();
}

std::string MDXGenerator::generateSeeAlsoSection(const codex::Symbol& sym,
                                                 const ParsedComment& parsed)
{
    std::ostringstream linkStream;

    // Related types from childSymbols
    for (codex::SymbolID childId : sym.childSymbols)
    {
        const codex::Symbol* child = m_db->findByID(childId);
        if (!child) continue;

        switch (child->symbolKind)
        {
            case codex::SymbolKind::Class:
            case codex::SymbolKind::Struct:
            case codex::SymbolKind::Enum:
            case codex::SymbolKind::TypeAlias:
            {
                std::string filename = CrossLinker::qualifiedToFilename(child->qualifiedName);
                linkStream << "- [" << child->name << "](/saturn/api_reference/" << filename
                           << ")\n";
                break;
            }
            default:
                break;
        }
    }

    // Base classes
    for (codex::SymbolID baseId : sym.baseClasses)
    {
        const codex::Symbol* base = m_db->findByID(baseId);
        if (base)
        {
            std::string filename = CrossLinker::qualifiedToFilename(base->qualifiedName);
            linkStream << "- [" << base->name << "](/saturn/api_reference/" << filename << ")\n";
        }
    }

    // @see entries from comment
    linkStream << formatSeeAlsoFromComment(parsed);

    std::string links = linkStream.str();
    if (links.empty()) return "";

    std::ostringstream oss;
    oss << "## See Also\n\n";
    oss << links;
    oss << "\n";

    return oss.str();
}

// Struct versions (no access specifier pairs)

std::string MDXGenerator::generateStructConstructorsSection(
    const std::vector<std::shared_ptr<codex::Node>>& ctors)
{
    if (ctors.empty()) return "";

    std::ostringstream oss;
    oss << "## Constructors\n\n";

    for (const auto& node : ctors)
    {
        auto* ctor = dynamic_cast<codex::ConstructorNode*>(node.get());
        if (!ctor) continue;

        ParsedComment parsed = parseNodeComment(node);
        std::string desc = !parsed.description.empty() ? parsed.description : parsed.brief;
        if (!desc.empty()) oss << desc << "\n\n";

        oss << "```cpp\n";
        oss << MDXFormatter::formatConstructorSignature(*ctor) << ";\n";
        oss << "```\n\n";

        if (!ctor->parameters.empty())
        {
            oss << "**Parameters:**\n\n";
            oss << MDXFormatter::formatParametersTable(ctor->parameters, m_linker, parsed.params);
            oss << "\n";
        }

        oss << MDXFormatter::formatNotesSection(parsed.notes);

        std::string seeAlso = formatSeeAlsoFromComment(parsed);
        if (!seeAlso.empty()) oss << "**See also:**\n\n" << seeAlso << "\n";
    }

    return oss.str();
}

std::string MDXGenerator::generateStructDestructorsSection(
    const std::vector<std::shared_ptr<codex::Node>>& dtors)
{
    if (dtors.empty()) return "";

    std::ostringstream oss;
    oss << "## Destructor\n\n";

    for (const auto& node : dtors)
    {
        auto* dtor = dynamic_cast<codex::DestructorNode*>(node.get());
        if (!dtor) continue;

        ParsedComment parsed = parseNodeComment(node);
        std::string desc = !parsed.description.empty() ? parsed.description : parsed.brief;
        if (!desc.empty()) oss << desc << "\n\n";

        oss << "```cpp\n";
        oss << MDXFormatter::formatDestructorSignature(*dtor) << ";\n";
        oss << "```\n\n";
    }

    return oss.str();
}

std::string MDXGenerator::generateStructMethodsSection(
    const std::vector<std::shared_ptr<codex::Node>>& methods)
{
    if (methods.empty()) return "";

    std::ostringstream oss;
    oss << "## Methods\n\n";

    for (const auto& node : methods)
    {
        auto* fn = dynamic_cast<codex::FunctionNode*>(node.get());
        if (!fn) continue;

        oss << "### " << fn->name << "\n\n";

        ParsedComment parsed = parseNodeComment(node);
        std::string desc = !parsed.description.empty() ? parsed.description : parsed.brief;
        if (!desc.empty()) oss << desc << "\n\n";

        oss << "```cpp\n";
        oss << MDXFormatter::formatFunctionSignature(*fn) << ";\n";
        oss << "```\n\n";

        if (!fn->parameters.empty())
        {
            oss << "**Parameters:**\n\n";
            oss << MDXFormatter::formatParametersTable(fn->parameters, m_linker, parsed.params);
            oss << "\n";
        }

        if (!fn->returnSignature.baseType.empty() && fn->returnSignature.baseType != "void")
        {
            oss << "**Returns:** "
                << MDXFormatter::formatReturnWithDescription(
                       m_linker.linkifyTypeSignature(fn->returnSignature), parsed.returns)
                << "\n\n";
        }

        oss << MDXFormatter::formatNotesSection(parsed.notes);

        std::string seeAlso = formatSeeAlsoFromComment(parsed);
        if (!seeAlso.empty()) oss << "**See also:**\n\n" << seeAlso << "\n";
    }

    return oss.str();
}

std::string MDXGenerator::generateStructOperatorsSection(
    const std::vector<std::shared_ptr<codex::Node>>& ops)
{
    if (ops.empty()) return "";

    std::ostringstream oss;
    oss << "## Operators\n\n";

    for (const auto& node : ops)
    {
        auto* op = dynamic_cast<codex::OperatorNode*>(node.get());
        if (!op) continue;

        oss << "### operator" << op->operatorSymbol << "\n\n";

        ParsedComment parsed = parseNodeComment(node);
        std::string desc = !parsed.description.empty() ? parsed.description : parsed.brief;
        if (!desc.empty()) oss << desc << "\n\n";

        oss << "```cpp\n";
        oss << MDXFormatter::formatOperatorSignature(*op) << ";\n";
        oss << "```\n\n";

        if (!op->parameters.empty())
        {
            oss << "**Parameters:**\n\n";
            oss << MDXFormatter::formatParametersTable(op->parameters, m_linker, parsed.params);
            oss << "\n";
        }

        if (!op->returnSignature.baseType.empty() && op->returnSignature.baseType != "void")
        {
            oss << "**Returns:** "
                << MDXFormatter::formatReturnWithDescription(
                       m_linker.linkifyTypeSignature(op->returnSignature), parsed.returns)
                << "\n\n";
        }

        oss << MDXFormatter::formatNotesSection(parsed.notes);

        std::string seeAlso = formatSeeAlsoFromComment(parsed);
        if (!seeAlso.empty()) oss << "**See also:**\n\n" << seeAlso << "\n";
    }

    return oss.str();
}

std::string MDXGenerator::generateStructMemberVarsSection(
    const std::vector<std::shared_ptr<codex::Node>>& vars)
{
    if (vars.empty()) return "";

    std::ostringstream oss;
    oss << "## Members\n\n";

    oss << "| Name | Type | Description |\n";
    oss << "|------|------|-------------|\n";

    for (const auto& node : vars)
    {
        auto* var = dynamic_cast<codex::VariableNode*>(node.get());
        if (!var) continue;

        oss << "| `" << var->name << "` | ";
        oss << m_linker.linkifyTypeSignature(var->typeSignature) << " | ";

        ParsedComment parsed = parseNodeComment(node);
        oss << parsed.brief << " |\n";
    }

    oss << "\n";
    return oss.str();
}

} // namespace docsgen
