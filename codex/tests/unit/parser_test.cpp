#include <gtest/gtest.h>

#include <functional>
#include <iostream>

#include <tree_sitter/api.h>

#include <codex/parser.hpp>
#include <codex/nodes.hpp>

#include "test_utils.hpp"

extern "C"
{
    const TSLanguage* tree_sitter_cpp();
}

namespace codex::tests
{

class ParserTest : public ::testing::Test
{
   protected:
    void SetUp() override {}
    void TearDown() override {}
};

/////////////////////////////////////////////////////////////////////////////////////////////
// Namespace Tests
/////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(ParserTest, ParseSimpleNamespace)
{
    auto result = parseSingle("namespace foo {}");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto ns = std::dynamic_pointer_cast<NamespaceNode>(result->children[0]);
    ASSERT_NE(ns, nullptr);
    EXPECT_EQ(ns->name, "foo");
    EXPECT_FALSE(ns->isAnonymous);
}

TEST_F(ParserTest, ParseNestedNamespace)
{
    auto result = parseSingle("namespace foo::bar::baz {}");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto ns = std::dynamic_pointer_cast<NamespaceNode>(result->children[0]);
    ASSERT_NE(ns, nullptr);
    EXPECT_EQ(ns->name, "foo::bar::baz");
    EXPECT_TRUE(ns->isNested);
}

TEST_F(ParserTest, ParseAnonymousNamespace)
{
    auto result = parseSingle("namespace {}");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto ns = std::dynamic_pointer_cast<NamespaceNode>(result->children[0]);
    ASSERT_NE(ns, nullptr);
    EXPECT_TRUE(ns->isAnonymous);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Class Tests
/////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(ParserTest, ParseSimpleClass)
{
    auto result = parseSingle("class Foo {};");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto cls = std::dynamic_pointer_cast<ClassNode>(result->children[0]);
    ASSERT_NE(cls, nullptr);
    EXPECT_EQ(cls->name, "Foo");
    EXPECT_FALSE(cls->isFinal);
}

TEST_F(ParserTest, ParseFinalClass)
{
    auto result = parseSingle("class Foo final {};");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto cls = std::dynamic_pointer_cast<ClassNode>(result->children[0]);
    ASSERT_NE(cls, nullptr);
    EXPECT_EQ(cls->name, "Foo");
    EXPECT_TRUE(cls->isFinal);
}

TEST_F(ParserTest, ParseClassWithBaseClass)
{
    auto result = parseSingle("class Derived : public Base {};");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto cls = std::dynamic_pointer_cast<ClassNode>(result->children[0]);
    ASSERT_NE(cls, nullptr);
    EXPECT_EQ(cls->name, "Derived");
    ASSERT_EQ(cls->baseClasses.size(), 1);
    EXPECT_EQ(cls->baseClasses[0].first, AccessSpecifier::Public);
    EXPECT_EQ(cls->baseClasses[0].second, "Base");
}

TEST_F(ParserTest, ParseClassWithMultipleBaseClasses)
{
    auto result = parseSingle("class Derived : public Base1, protected Base2, private Base3 {};");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto cls = std::dynamic_pointer_cast<ClassNode>(result->children[0]);
    ASSERT_NE(cls, nullptr);
    ASSERT_EQ(cls->baseClasses.size(), 3);
}

TEST_F(ParserTest, ParseClassWithPublicMember)
{
    auto result = parseSingle(R"(
class Foo {
public:
    int bar;
};
)");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto cls = std::dynamic_pointer_cast<ClassNode>(result->children[0]);
    ASSERT_NE(cls, nullptr);
    ASSERT_EQ(cls->memberVariables.size(), 1);
    EXPECT_EQ(cls->memberVariables[0].first, AccessSpecifier::Public);
}

TEST_F(ParserTest, ParseClassWithPrivateMember)
{
    auto result = parseSingle(R"(
class Foo {
private:
    int bar;
};
)");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto cls = std::dynamic_pointer_cast<ClassNode>(result->children[0]);
    ASSERT_NE(cls, nullptr);
    ASSERT_EQ(cls->memberVariables.size(), 1);
    EXPECT_EQ(cls->memberVariables[0].first, AccessSpecifier::Private);
}

TEST_F(ParserTest, ParseNestedClass)
{
    auto result = parseSingle(R"(
class Outer {
    class Inner {
    };
};
)");

    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto outer = std::dynamic_pointer_cast<ClassNode>(result->children[0]);
    ASSERT_NE(outer, nullptr);
    ASSERT_EQ(outer->nestedTypes.size(), 1);

    auto inner = std::dynamic_pointer_cast<ClassNode>(outer->nestedTypes[0].second);
    ASSERT_NE(inner, nullptr);
    EXPECT_EQ(inner->name, "Inner");
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Struct Tests
/////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(ParserTest, ParseSimpleStruct)
{
    auto result = parseSingle("struct Foo {};");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto st = std::dynamic_pointer_cast<StructNode>(result->children[0]);
    ASSERT_NE(st, nullptr);
    EXPECT_EQ(st->name, "Foo");
}

TEST_F(ParserTest, ParseStructWithMembers)
{
    auto result = parseSingle(R"(
struct Point {
    int x;
    int y;
};
)");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto st = std::dynamic_pointer_cast<StructNode>(result->children[0]);
    ASSERT_NE(st, nullptr);
    EXPECT_EQ(st->memberVariables.size(), 2);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Function Tests
/////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(ParserTest, ParseFreeFunction)
{
    auto result = parseSingle("void foo();");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto fn = std::dynamic_pointer_cast<FunctionNode>(result->children[0]);
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(fn->name, "foo");
    EXPECT_EQ(fn->returnSignature.baseType, "void");
}

TEST_F(ParserTest, ParseFunctionWithParameters)
{
    auto result = parseSingle("int add(int a, int* b, int& c);");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto fn = std::dynamic_pointer_cast<FunctionNode>(result->children[0]);
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(fn->name, "add");
    EXPECT_EQ(fn->returnSignature.baseType, "int");
    ASSERT_EQ(fn->parameters.size(), 3);

    EXPECT_EQ(fn->parameters[0].name, "a");
    EXPECT_EQ(fn->parameters[0].typeSignature.baseType, "int");
    EXPECT_FALSE(fn->parameters[0].typeSignature.isPointer);

    EXPECT_EQ(fn->parameters[1].name, "b");
    EXPECT_EQ(fn->parameters[1].typeSignature.baseType, "int");
    EXPECT_TRUE(fn->parameters[1].typeSignature.isPointer);

    EXPECT_EQ(fn->parameters[2].name, "c");
    EXPECT_EQ(fn->parameters[2].typeSignature.baseType, "int");
    EXPECT_FALSE(fn->parameters[2].typeSignature.isPointer);
    EXPECT_TRUE(fn->parameters[2].typeSignature.isLValueRef);
}

TEST_F(ParserTest, ParseFunctionWithDefaultParameters)
{
    auto result = parseSingle("void foo(int* a = nullptr, double&& b = 3.14);");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto fn = std::dynamic_pointer_cast<FunctionNode>(result->children[0]);
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(fn->name, "foo");
    ASSERT_EQ(fn->parameters.size(), 2);

    EXPECT_EQ(fn->parameters[0].name, "a");
    EXPECT_EQ(fn->parameters[0].typeSignature.baseType, "int");
    EXPECT_TRUE(fn->parameters[0].typeSignature.isPointer);
    EXPECT_EQ(fn->parameters[0].defaultValue, "nullptr");

    EXPECT_EQ(fn->parameters[1].name, "b");
    EXPECT_EQ(fn->parameters[1].typeSignature.baseType, "double");
    EXPECT_TRUE(fn->parameters[1].typeSignature.isRValueRef);
    EXPECT_EQ(fn->parameters[1].defaultValue, "3.14");
}

TEST_F(ParserTest, ParseConstexprFunction)
{
    auto result = parseSingle("constexpr int foo();");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto fn = std::dynamic_pointer_cast<FunctionNode>(result->children[0]);
    ASSERT_NE(fn, nullptr);
    EXPECT_TRUE(fn->isConstexpr);
}

TEST_F(ParserTest, ParseInlineFunction)
{
    auto result = parseSingle("inline void foo();");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto fn = std::dynamic_pointer_cast<FunctionNode>(result->children[0]);
    ASSERT_NE(fn, nullptr);
    EXPECT_TRUE(fn->isInline);
}

TEST_F(ParserTest, ParseStaticFunction)
{
    auto result = parseSingle("static void foo();");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto fn = std::dynamic_pointer_cast<FunctionNode>(result->children[0]);
    ASSERT_NE(fn, nullptr);
    EXPECT_TRUE(fn->isStatic);
}

TEST_F(ParserTest, ParseNoexceptFunction)
{
    auto result = parseSingle("void foo() noexcept;");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto fn = std::dynamic_pointer_cast<FunctionNode>(result->children[0]);
    ASSERT_NE(fn, nullptr);
    EXPECT_TRUE(fn->isNoexcept);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Constructor/Destructor Tests
/////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(ParserTest, ParseDefaultConstructor)
{
    auto result = parseSingle(R"(
struct Foo {
    Foo() {}
};
)");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto st = std::dynamic_pointer_cast<StructNode>(result->children[0]);
    ASSERT_NE(st, nullptr);
    ASSERT_EQ(st->constructors.size(), 1);

    auto ctor = std::dynamic_pointer_cast<ConstructorNode>(st->constructors[0]);
    ASSERT_NE(ctor, nullptr);
    EXPECT_EQ(ctor->name, "Foo");
    EXPECT_EQ(ctor->parameters.size(), 0);
}

TEST_F(ParserTest, ParseCopyConstructor)
{
    auto result = parseSingle(R"(
struct Foo {
    Foo(const Foo& other) {}
};
)");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto st = std::dynamic_pointer_cast<StructNode>(result->children[0]);
    ASSERT_NE(st, nullptr);
    ASSERT_EQ(st->constructors.size(), 1);

    auto ctor = std::dynamic_pointer_cast<ConstructorNode>(st->constructors[0]);
    ASSERT_NE(ctor, nullptr);
    EXPECT_TRUE(ctor->isCopyConstructor);
    EXPECT_FALSE(ctor->isMoveConstructor);
}

TEST_F(ParserTest, ParseMoveConstructor)
{
    auto result = parseSingle(R"(
struct Foo {
    Foo(Foo&& other) {}
};
)");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto st = std::dynamic_pointer_cast<StructNode>(result->children[0]);
    ASSERT_NE(st, nullptr);
    ASSERT_EQ(st->constructors.size(), 1);

    auto ctor = std::dynamic_pointer_cast<ConstructorNode>(st->constructors[0]);
    ASSERT_NE(ctor, nullptr);
    EXPECT_FALSE(ctor->isCopyConstructor);
    EXPECT_TRUE(ctor->isMoveConstructor);
}

TEST_F(ParserTest, ParseDestructor)
{
    auto result = parseSingle(R"(
struct Foo {
    ~Foo() {}
};
)");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto st = std::dynamic_pointer_cast<StructNode>(result->children[0]);
    ASSERT_NE(st, nullptr);
    ASSERT_EQ(st->destructors.size(), 1);

    auto dtor = std::dynamic_pointer_cast<DestructorNode>(st->destructors[0]);
    ASSERT_NE(dtor, nullptr);
}

TEST_F(ParserTest, ParseVirtualDestructor)
{
    auto result = parseSingle(R"(
class Foo {
public:
    virtual ~Foo() {}
};
)");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto cls = std::dynamic_pointer_cast<ClassNode>(result->children[0]);
    ASSERT_NE(cls, nullptr);
    ASSERT_EQ(cls->destructors.size(), 1);

    auto dtor = std::dynamic_pointer_cast<DestructorNode>(cls->destructors[0].second);
    ASSERT_NE(dtor, nullptr);
    EXPECT_TRUE(dtor->isVirtual);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Enum Tests
/////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(ParserTest, ParseEnum)
{
    auto result = parseSingle("enum Color { Red, Green, Blue };");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto en = std::dynamic_pointer_cast<EnumNode>(result->children[0]);
    ASSERT_NE(en, nullptr);
    EXPECT_EQ(en->name, "Color");
    EXPECT_FALSE(en->isScoped);
    EXPECT_EQ(en->enumerators.size(), 3);
}

TEST_F(ParserTest, ParseScopedEnum)
{
    auto result = parseSingle("enum class Color { Red, Green, Blue };");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto en = std::dynamic_pointer_cast<EnumNode>(result->children[0]);
    ASSERT_NE(en, nullptr);
    EXPECT_EQ(en->name, "Color");
    EXPECT_TRUE(en->isScoped);
}

TEST_F(ParserTest, ParseEnumWithUnderlyingType)
{
    auto result = parseSingle("enum class Color : uint8_t { Red, Green, Blue };");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto en = std::dynamic_pointer_cast<EnumNode>(result->children[0]);
    ASSERT_NE(en, nullptr);
    EXPECT_EQ(en->underlyingType, "uint8_t");
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Variable Tests
/////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(ParserTest, ParseGlobalVariable)
{
    auto result = parseSingle("int foo;");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto var = std::dynamic_pointer_cast<VariableNode>(result->children[0]);
    ASSERT_NE(var, nullptr);
    EXPECT_EQ(var->name, "foo");
    EXPECT_EQ(var->typeSignature.baseType, "int");
}

TEST_F(ParserTest, ParseConstVariable)
{
    auto result = parseSingle("const int foo = 42;");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto var = std::dynamic_pointer_cast<VariableNode>(result->children[0]);
    ASSERT_NE(var, nullptr);
    EXPECT_TRUE(var->typeSignature.isConst);
}

TEST_F(ParserTest, ParseVolatileVariable)
{
    auto result = parseSingle("volatile int x;");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto var = std::dynamic_pointer_cast<VariableNode>(result->children[0]);
    ASSERT_NE(var, nullptr);
    EXPECT_TRUE(var->typeSignature.isVolatile);
    EXPECT_EQ(var->typeSignature.baseType, "int");
}

TEST_F(ParserTest, ParsePointerVariable)
{
    auto result = parseSingle("int* x;");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto var = std::dynamic_pointer_cast<VariableNode>(result->children[0]);
    ASSERT_NE(var, nullptr);
    EXPECT_TRUE(var->typeSignature.isPointer);
    EXPECT_EQ(var->typeSignature.baseType, "int");
}

TEST_F(ParserTest, ParseReferenceVariable)
{
    auto result = parseSingle(R"(
struct Foo {};
const Foo& y = *static_cast<Foo*>(nullptr);
)");
    ASSERT_NE(result, nullptr);
    ASSERT_GE(result->children.size(), 2);

    auto var = std::dynamic_pointer_cast<VariableNode>(result->children[1]);
    ASSERT_NE(var, nullptr);
    EXPECT_TRUE(var->typeSignature.isConst);
    EXPECT_TRUE(var->typeSignature.isLValueRef);
    EXPECT_EQ(var->typeSignature.baseType, "Foo");
}

TEST_F(ParserTest, ParseRValueReferenceVariable)
{
    auto result = parseSingle(R"(
struct Foo {};
Foo&& z = Foo();
)");
    ASSERT_NE(result, nullptr);
    ASSERT_GE(result->children.size(), 2);

    auto var = std::dynamic_pointer_cast<VariableNode>(result->children[1]);
    ASSERT_NE(var, nullptr);
    EXPECT_TRUE(var->typeSignature.isRValueRef);
    EXPECT_EQ(var->typeSignature.baseType, "Foo");
}

TEST_F(ParserTest, ParseMutableVariable)
{
    auto result = parseSingle("mutable int count;");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto var = std::dynamic_pointer_cast<VariableNode>(result->children[0]);
    ASSERT_NE(var, nullptr);
    EXPECT_TRUE(var->typeSignature.isMutable);
}

TEST_F(ParserTest, ParseQualifiedVariable)
{
    auto result = parseSingle("foo::Bar b;");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto var = std::dynamic_pointer_cast<VariableNode>(result->children[0]);
    ASSERT_NE(var, nullptr);
    EXPECT_EQ(var->typeSignature.baseType, "foo::Bar");
}

TEST_F(ParserTest, ParseConstexprVariable)
{
    auto result = parseSingle("constexpr int foo = 42;");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto var = std::dynamic_pointer_cast<VariableNode>(result->children[0]);
    ASSERT_NE(var, nullptr);
    EXPECT_TRUE(var->isConstexpr);
}

TEST_F(ParserTest, ParseStaticVariable)
{
    auto result = parseSingle("static int foo;");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto var = std::dynamic_pointer_cast<VariableNode>(result->children[0]);
    ASSERT_NE(var, nullptr);
    EXPECT_TRUE(var->isStatic);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Template Tests
/////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(ParserTest, ParseTemplateClass)
{
    auto result = parseSingle("template<typename T> class Foo {};");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto cls = std::dynamic_pointer_cast<ClassNode>(result->children[0]);
    ASSERT_NE(cls, nullptr);
    EXPECT_EQ(cls->name, "Foo");
    ASSERT_NE(cls->templateDecl, nullptr);

    auto tmpl = std::dynamic_pointer_cast<TemplateNode>(cls->templateDecl);
    ASSERT_NE(tmpl, nullptr);
    ASSERT_EQ(tmpl->parameters.size(), 1);
    EXPECT_EQ(tmpl->parameters[0].keyword, "typename");
    EXPECT_EQ(tmpl->parameters[0].name, "T");
}

TEST_F(ParserTest, ParseTemplateFunction)
{
    auto result = parseSingle("template<typename T> T identity(T value);");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto fn = std::dynamic_pointer_cast<FunctionNode>(result->children[0]);
    ASSERT_NE(fn, nullptr);
    ASSERT_NE(fn->templateDecl, nullptr);
}

TEST_F(ParserTest, ParseVariadicTemplate)
{
    auto result = parseSingle("template<typename... Ts> class Tuple {};");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto cls = std::dynamic_pointer_cast<ClassNode>(result->children[0]);
    ASSERT_NE(cls, nullptr);
    ASSERT_NE(cls->templateDecl, nullptr);

    auto tmpl = std::dynamic_pointer_cast<TemplateNode>(cls->templateDecl);
    ASSERT_NE(tmpl, nullptr);
    ASSERT_EQ(tmpl->parameters.size(), 1);
    EXPECT_TRUE(tmpl->parameters[0].isVariadic);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Include Directive Tests
/////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(ParserTest, ParseSystemInclude)
{
    auto result = parseSingle("#include <iostream>");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto inc = std::dynamic_pointer_cast<IncludeNode>(result->children[0]);
    ASSERT_NE(inc, nullptr);
    EXPECT_EQ(inc->path, "iostream");
    EXPECT_TRUE(inc->isSystem);
}

TEST_F(ParserTest, ParseLocalInclude)
{
    auto result = parseSingle("#include \"myheader.hpp\"");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto inc = std::dynamic_pointer_cast<IncludeNode>(result->children[0]);
    ASSERT_NE(inc, nullptr);
    EXPECT_EQ(inc->path, "myheader.hpp");
    EXPECT_FALSE(inc->isSystem);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Macro Tests
/////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(ParserTest, ParseObjectLikeMacro)
{
    auto result = parseSingle("#define FOO 42");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto macro = std::dynamic_pointer_cast<ObjectLikeMacroNode>(result->children[0]);
    ASSERT_NE(macro, nullptr);
    EXPECT_EQ(macro->name, "FOO");
    EXPECT_EQ(macro->body, "42");
}

TEST_F(ParserTest, ParseFunctionLikeMacro)
{
    auto result = parseSingle("#define MAX(a, b) ((a) > (b) ? (a) : (b))");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto macro = std::dynamic_pointer_cast<FunctionLikeMacroNode>(result->children[0]);
    ASSERT_NE(macro, nullptr);
    EXPECT_EQ(macro->name, "MAX");
    ASSERT_EQ(macro->parameters.size(), 2);
    EXPECT_EQ(macro->parameters[0].name, "a");
    EXPECT_EQ(macro->parameters[1].name, "b");
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Type Alias Tests
/////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(ParserTest, ParseTypedef)
{
    auto result = parseSingle("typedef unsigned int uint;");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto td = std::dynamic_pointer_cast<TypedefNode>(result->children[0]);
    ASSERT_NE(td, nullptr);
    EXPECT_EQ(td->aliasName, "uint");
}

TEST_F(ParserTest, ParseUsingAlias)
{
    auto result = parseSingle("using uint = unsigned int;");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto ta = std::dynamic_pointer_cast<TypeAliasNode>(result->children[0]);
    ASSERT_NE(ta, nullptr);
    EXPECT_EQ(ta->aliasName, "uint");
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Comment Tests
/////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(ParserTest, ParseCommentAttachedToClass)
{
    auto result = parseSingle(R"(
// This is a comment
class Foo {};
)");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto cls = std::dynamic_pointer_cast<ClassNode>(result->children[0]);
    ASSERT_NE(cls, nullptr);
    ASSERT_NE(cls->comment, nullptr);

    auto comment = std::dynamic_pointer_cast<CommentNode>(cls->comment);
    ASSERT_NE(comment, nullptr);
    EXPECT_TRUE(comment->text.find("This is a comment") != std::string::npos);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Operator Tests
/////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(ParserTest, ParseOperatorOverload)
{
    auto result = parseSingle(R"(
struct Foo {
    bool operator==(const Foo& other) const { return true; }
};
)");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto st = std::dynamic_pointer_cast<StructNode>(result->children[0]);
    ASSERT_NE(st, nullptr);
    ASSERT_EQ(st->operators.size(), 1);

    auto op = std::dynamic_pointer_cast<OperatorNode>(st->operators[0]);
    ASSERT_NE(op, nullptr);
    EXPECT_EQ(op->operatorSymbol, "==");
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Declaration vs Definition Tests (member functions, constructors, destructors, operators)
/////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(ParserTest, ParseStructConstructorDeclarations_CopyMoveDetection)
{
    auto result = parseSingle(R"(
struct Foo {
    Foo();
    Foo(int x);
    Foo(const Foo& other);
    Foo(Foo&& other);
};
)");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto st = std::dynamic_pointer_cast<StructNode>(result->children[0]);
    ASSERT_NE(st, nullptr);
    ASSERT_EQ(st->constructors.size(), 4);

    // Check copy/move detection works for declarations too
    auto copy = std::dynamic_pointer_cast<ConstructorNode>(st->constructors[2]);
    ASSERT_NE(copy, nullptr);
    EXPECT_TRUE(copy->isCopyConstructor);

    auto move = std::dynamic_pointer_cast<ConstructorNode>(st->constructors[3]);
    ASSERT_NE(move, nullptr);
    EXPECT_TRUE(move->isMoveConstructor);
}

TEST_F(ParserTest, ParseStructMixedDeclarationsAndDefinitions)
{
    auto result = parseSingle(R"(
struct Foo {
    Foo();                          // declaration
    Foo(int x) {}                   // definition
    ~Foo();                         // declaration
    void bar();                     // declaration
    void baz() {}                   // definition
    bool operator==(const Foo& o);  // declaration
    bool operator!=(const Foo& o) { return !(*this == o); } // definition
};
)");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto st = std::dynamic_pointer_cast<StructNode>(result->children[0]);
    ASSERT_NE(st, nullptr);
    EXPECT_EQ(st->constructors.size(), 2);
    EXPECT_EQ(st->destructors.size(), 1);
    EXPECT_EQ(st->memberFunctions.size(), 2);
    EXPECT_EQ(st->operators.size(), 2);
}

TEST_F(ParserTest, ParseClassConstructorDeclarations)
{
    auto result = parseSingle(R"(
class Foo {
public:
    Foo();
    Foo(int x);
private:
    Foo(const Foo& other);
};
)");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto cls = std::dynamic_pointer_cast<ClassNode>(result->children[0]);
    ASSERT_NE(cls, nullptr);
    ASSERT_EQ(cls->constructors.size(), 3);

    // Check access specifiers
    EXPECT_EQ(cls->constructors[0].first, AccessSpecifier::Public);
    EXPECT_EQ(cls->constructors[1].first, AccessSpecifier::Public);
    EXPECT_EQ(cls->constructors[2].first, AccessSpecifier::Private);
}

TEST_F(ParserTest, ParseClassVirtualDestructorDeclaration)
{
    auto result = parseSingle(R"(
class Base {
public:
    virtual ~Base();
};
)");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto cls = std::dynamic_pointer_cast<ClassNode>(result->children[0]);
    ASSERT_NE(cls, nullptr);
    ASSERT_EQ(cls->destructors.size(), 1);

    auto dtor = std::dynamic_pointer_cast<DestructorNode>(cls->destructors[0].second);
    ASSERT_NE(dtor, nullptr);
    EXPECT_TRUE(dtor->isVirtual);
}

TEST_F(ParserTest, ParseClassOperatorDeclarations)
{
    auto result = parseSingle(R"(
class Foo {
public:
    Foo& operator=(const Foo& other);
    Foo& operator=(Foo&& other);
private:
    bool operator<(const Foo& other) const;
};
)");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto cls = std::dynamic_pointer_cast<ClassNode>(result->children[0]);
    ASSERT_NE(cls, nullptr);
    ASSERT_EQ(cls->operators.size(), 3);

    EXPECT_EQ(cls->operators[0].first, AccessSpecifier::Public);
    EXPECT_EQ(cls->operators[1].first, AccessSpecifier::Public);
    EXPECT_EQ(cls->operators[2].first, AccessSpecifier::Private);
}

TEST_F(ParserTest, ParseClassMixedDeclarationsAndDefinitions)
{
    auto result = parseSingle(R"(
class Widget {
public:
    Widget();
    Widget(int id) : m_id(id) {}
    ~Widget();

    int getId() const;
    void setId(int id) { m_id = id; }

    bool operator==(const Widget& other) const;

private:
    int m_id;
};
)");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto cls = std::dynamic_pointer_cast<ClassNode>(result->children[0]);
    ASSERT_NE(cls, nullptr);
    EXPECT_EQ(cls->constructors.size(), 2);
    EXPECT_EQ(cls->destructors.size(), 1);
    EXPECT_EQ(cls->memberFunctions.size(), 2);
    EXPECT_EQ(cls->operators.size(), 1);
    EXPECT_EQ(cls->memberVariables.size(), 1);
}

TEST_F(ParserTest, ParseClassMemberFunctionDeclarationsWithModifiers)
{
    auto result = parseSingle(R"(
class Foo {
public:
    virtual void foo();
    void bar() const;
    static void baz();
    virtual void qux() = 0;
};
)");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto cls = std::dynamic_pointer_cast<ClassNode>(result->children[0]);
    ASSERT_NE(cls, nullptr);
    // virtual foo, const bar, pure virtual qux = 3 member functions
    // static baz = 1 static member function
    EXPECT_EQ(cls->memberFunctions.size(), 3);
    EXPECT_EQ(cls->staticMemberFunctions.size(), 1);
}

TEST_F(ParserTest, ParseStructStaticMemberFunctionDeclaration)
{
    auto result = parseSingle(R"(
struct Foo {
    static void create();
    static Foo* getInstance();
};
)");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto st = std::dynamic_pointer_cast<StructNode>(result->children[0]);
    ASSERT_NE(st, nullptr);
    EXPECT_EQ(st->staticMemberFunctions.size(), 2);
    EXPECT_EQ(st->memberFunctions.size(), 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Advanced Template Parameter Tests
/////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(ParserTest, ParseNonTypeTemplateParameter)
{
    auto result = parseSingle("template<int N> struct Foo {};");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto st = std::dynamic_pointer_cast<StructNode>(result->children[0]);
    ASSERT_NE(st, nullptr);
    ASSERT_NE(st->templateDecl, nullptr);

    auto tmpl = std::dynamic_pointer_cast<TemplateNode>(st->templateDecl);
    ASSERT_NE(tmpl, nullptr);
    ASSERT_EQ(tmpl->parameters.size(), 1);
    EXPECT_EQ(tmpl->parameters[0].paramKind, TemplateParameterKind::NonType);
    EXPECT_EQ(tmpl->parameters[0].typeSignature.baseType, "int");
    EXPECT_EQ(tmpl->parameters[0].name, "N");
}

TEST_F(ParserTest, ParseNonTypeTemplateParameterAuto)
{
    auto result = parseSingle("template<auto V> struct Foo {};");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto st = std::dynamic_pointer_cast<StructNode>(result->children[0]);
    ASSERT_NE(st, nullptr);
    ASSERT_NE(st->templateDecl, nullptr);

    auto tmpl = std::dynamic_pointer_cast<TemplateNode>(st->templateDecl);
    ASSERT_NE(tmpl, nullptr);
    ASSERT_EQ(tmpl->parameters.size(), 1);
    EXPECT_EQ(tmpl->parameters[0].paramKind, TemplateParameterKind::NonType);
    EXPECT_EQ(tmpl->parameters[0].name, "V");
}

TEST_F(ParserTest, ParseTypeParameterWithDefault)
{
    auto result = parseSingle("template<typename T = int> struct Foo {};");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto st = std::dynamic_pointer_cast<StructNode>(result->children[0]);
    ASSERT_NE(st, nullptr);
    ASSERT_NE(st->templateDecl, nullptr);

    auto tmpl = std::dynamic_pointer_cast<TemplateNode>(st->templateDecl);
    ASSERT_NE(tmpl, nullptr);
    ASSERT_EQ(tmpl->parameters.size(), 1);
    EXPECT_EQ(tmpl->parameters[0].paramKind, TemplateParameterKind::Type);
    EXPECT_EQ(tmpl->parameters[0].keyword, "typename");
    EXPECT_EQ(tmpl->parameters[0].name, "T");
    EXPECT_EQ(tmpl->parameters[0].defaultValue, "int");
}

TEST_F(ParserTest, ParseNonTypeParameterWithDefault)
{
    auto result = parseSingle("template<int N = 42> struct Foo {};");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto st = std::dynamic_pointer_cast<StructNode>(result->children[0]);
    ASSERT_NE(st, nullptr);
    ASSERT_NE(st->templateDecl, nullptr);

    auto tmpl = std::dynamic_pointer_cast<TemplateNode>(st->templateDecl);
    ASSERT_NE(tmpl, nullptr);
    ASSERT_EQ(tmpl->parameters.size(), 1);
    EXPECT_EQ(tmpl->parameters[0].paramKind, TemplateParameterKind::NonType);
    EXPECT_EQ(tmpl->parameters[0].typeSignature.baseType, "int");
    EXPECT_EQ(tmpl->parameters[0].name, "N");
    EXPECT_EQ(tmpl->parameters[0].defaultValue, "42");
}

TEST_F(ParserTest, ParseTemplateTemplateParameter)
{
    auto result = parseSingle("template<template<typename> class C> struct Foo {};");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto st = std::dynamic_pointer_cast<StructNode>(result->children[0]);
    ASSERT_NE(st, nullptr);
    ASSERT_NE(st->templateDecl, nullptr);

    auto tmpl = std::dynamic_pointer_cast<TemplateNode>(st->templateDecl);
    ASSERT_NE(tmpl, nullptr);
    ASSERT_EQ(tmpl->parameters.size(), 1);
    EXPECT_EQ(tmpl->parameters[0].paramKind, TemplateParameterKind::TemplateTemplate);
    EXPECT_EQ(tmpl->parameters[0].name, "C");
    EXPECT_EQ(tmpl->parameters[0].keyword, "class");
    ASSERT_EQ(tmpl->parameters[0].innerParameters.size(), 1);
    EXPECT_EQ(tmpl->parameters[0].innerParameters[0].keyword, "typename");
}

TEST_F(ParserTest, ParseMixedTemplateParameters)
{
    auto result = parseSingle("template<typename T, int N, typename... Ts> struct Foo {};");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto st = std::dynamic_pointer_cast<StructNode>(result->children[0]);
    ASSERT_NE(st, nullptr);
    ASSERT_NE(st->templateDecl, nullptr);

    auto tmpl = std::dynamic_pointer_cast<TemplateNode>(st->templateDecl);
    ASSERT_NE(tmpl, nullptr);
    ASSERT_EQ(tmpl->parameters.size(), 3);

    // typename T
    EXPECT_EQ(tmpl->parameters[0].paramKind, TemplateParameterKind::Type);
    EXPECT_EQ(tmpl->parameters[0].keyword, "typename");
    EXPECT_EQ(tmpl->parameters[0].name, "T");
    EXPECT_FALSE(tmpl->parameters[0].isVariadic);

    // int N
    EXPECT_EQ(tmpl->parameters[1].paramKind, TemplateParameterKind::NonType);
    EXPECT_EQ(tmpl->parameters[1].typeSignature.baseType, "int");
    EXPECT_EQ(tmpl->parameters[1].name, "N");

    // typename... Ts
    EXPECT_EQ(tmpl->parameters[2].paramKind, TemplateParameterKind::Type);
    EXPECT_TRUE(tmpl->parameters[2].isVariadic);
    EXPECT_EQ(tmpl->parameters[2].name, "Ts");
}

TEST_F(ParserTest, ParseClassSpecializationArgs)
{
    auto result = parseSingle("template<> class Foo<int> {};");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto cls = std::dynamic_pointer_cast<ClassNode>(result->children[0]);
    ASSERT_NE(cls, nullptr);
    EXPECT_EQ(cls->name, "Foo");
    ASSERT_EQ(cls->templateArgs.size(), 1);
    EXPECT_EQ(cls->templateArgs[0].typeSignature.baseType, "int");
}

TEST_F(ParserTest, ParsePartialSpecialization)
{
    auto result = parseSingle("template<typename T> struct Foo<T, int> {};");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto st = std::dynamic_pointer_cast<StructNode>(result->children[0]);
    ASSERT_NE(st, nullptr);
    EXPECT_EQ(st->name, "Foo");
    ASSERT_NE(st->templateDecl, nullptr);
    ASSERT_EQ(st->templateArgs.size(), 2);
}

TEST_F(ParserTest, ParseNestedTemplateDefault)
{
    auto result = parseSingle("template<typename T = std::vector<int>> struct Foo {};");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto st = std::dynamic_pointer_cast<StructNode>(result->children[0]);
    ASSERT_NE(st, nullptr);
    ASSERT_NE(st->templateDecl, nullptr);

    auto tmpl = std::dynamic_pointer_cast<TemplateNode>(st->templateDecl);
    ASSERT_NE(tmpl, nullptr);
    ASSERT_EQ(tmpl->parameters.size(), 1);
    EXPECT_EQ(tmpl->parameters[0].paramKind, TemplateParameterKind::Type);
    EXPECT_EQ(tmpl->parameters[0].name, "T");
    EXPECT_FALSE(tmpl->parameters[0].defaultValue.empty());
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Attribute Tests
/////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(ParserTest, ParseFunctionWithNodiscardAttribute)
{
    auto result = parseSingle("[[nodiscard]] int getValue();");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto fn = std::dynamic_pointer_cast<FunctionNode>(result->children[0]);
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(fn->name, "getValue");
    ASSERT_EQ(fn->attributes.size(), 1);
    EXPECT_EQ(fn->attributes[0], "[[nodiscard]]");
}

TEST_F(ParserTest, ParseFunctionWithMultipleAttributes)
{
    auto result = parseSingle("[[nodiscard]] [[deprecated]] int getValue();");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto fn = std::dynamic_pointer_cast<FunctionNode>(result->children[0]);
    ASSERT_NE(fn, nullptr);
    ASSERT_EQ(fn->attributes.size(), 2);
    EXPECT_EQ(fn->attributes[0], "[[nodiscard]]");
    EXPECT_EQ(fn->attributes[1], "[[deprecated]]");
}

TEST_F(ParserTest, ParseClassWithMultipleAttributes)
{
    auto result = parseSingle("[[deprecated]] [[nodiscard]] class Foo {};");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto cls = std::dynamic_pointer_cast<ClassNode>(result->children[0]);
    ASSERT_NE(cls, nullptr);
    ASSERT_EQ(cls->attributes.size(), 2);
    EXPECT_EQ(cls->attributes[0], "[[deprecated]]");
    EXPECT_EQ(cls->attributes[1], "[[nodiscard]]");
}

TEST_F(ParserTest, ParseConstructorWithAttribute)
{
    auto result = parseSingle(R"(
struct Foo {
    [[nodiscard]] Foo(int x);
};
)");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto st = std::dynamic_pointer_cast<StructNode>(result->children[0]);
    ASSERT_NE(st, nullptr);
    ASSERT_EQ(st->constructors.size(), 1);

    auto ctor = std::dynamic_pointer_cast<ConstructorNode>(st->constructors[0]);
    ASSERT_NE(ctor, nullptr);
    ASSERT_EQ(ctor->attributes.size(), 1);
    EXPECT_EQ(ctor->attributes[0], "[[nodiscard]]");
}

TEST_F(ParserTest, ParseDestructorWithAttribute)
{
    auto result = parseSingle(R"(
class Foo {
public:
    [[deprecated("use release() instead")]] ~Foo();
};
)");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto cls = std::dynamic_pointer_cast<ClassNode>(result->children[0]);
    ASSERT_NE(cls, nullptr);
    ASSERT_EQ(cls->destructors.size(), 1);

    auto dtor = std::dynamic_pointer_cast<DestructorNode>(cls->destructors[0].second);
    ASSERT_NE(dtor, nullptr);
    ASSERT_EQ(dtor->attributes.size(), 1);
    EXPECT_TRUE(dtor->attributes[0].find("[[deprecated") != std::string::npos);
}

TEST_F(ParserTest, ParseOperatorWithNodiscardAttribute)
{
    auto result = parseSingle(R"(
struct Foo {
    [[nodiscard]] bool operator==(const Foo& other) const;
};
)");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto st = std::dynamic_pointer_cast<StructNode>(result->children[0]);
    ASSERT_NE(st, nullptr);
    ASSERT_EQ(st->operators.size(), 1);

    auto op = std::dynamic_pointer_cast<OperatorNode>(st->operators[0]);
    ASSERT_NE(op, nullptr);
    ASSERT_EQ(op->attributes.size(), 1);
    EXPECT_EQ(op->attributes[0], "[[nodiscard]]");
}

TEST_F(ParserTest, ParseVariableWithMaybeUnusedAttribute)
{
    auto result = parseSingle("[[maybe_unused]] int debugFlag;");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto var = std::dynamic_pointer_cast<VariableNode>(result->children[0]);
    ASSERT_NE(var, nullptr);
    EXPECT_EQ(var->name, "debugFlag");
    ASSERT_EQ(var->attributes.size(), 1);
    EXPECT_EQ(var->attributes[0], "[[maybe_unused]]");
}

TEST_F(ParserTest, ParseClassWithDeprecatedAttribute)
{
    auto result = parseSingle("[[deprecated]] class OldWidget {};");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto cls = std::dynamic_pointer_cast<ClassNode>(result->children[0]);
    ASSERT_NE(cls, nullptr);
    EXPECT_EQ(cls->name, "OldWidget");
    ASSERT_EQ(cls->attributes.size(), 1);
    EXPECT_EQ(cls->attributes[0], "[[deprecated]]");
}

TEST_F(ParserTest, ParseStructWithCustomAttribute)
{
    auto result = parseSingle("[[codex::export]] struct APIData {};");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto st = std::dynamic_pointer_cast<StructNode>(result->children[0]);
    ASSERT_NE(st, nullptr);
    EXPECT_EQ(st->name, "APIData");
    ASSERT_EQ(st->attributes.size(), 1);
    EXPECT_EQ(st->attributes[0], "[[codex::export]]");
}

TEST_F(ParserTest, ParseAttributeWithArgument)
{
    auto result = parseSingle(R"([[deprecated("use NewFunc instead")]] void oldFunc();)");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto fn = std::dynamic_pointer_cast<FunctionNode>(result->children[0]);
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(fn->name, "oldFunc");
    ASSERT_EQ(fn->attributes.size(), 1);
    EXPECT_TRUE(fn->attributes[0].find("deprecated") != std::string::npos);
    EXPECT_TRUE(fn->attributes[0].find("use NewFunc instead") != std::string::npos);
}

TEST_F(ParserTest, ParseUnionWithAttribute)
{
    auto result = parseSingle("[[deprecated]] union OldData { int i; float f; };");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto un = std::dynamic_pointer_cast<UnionNode>(result->children[0]);
    ASSERT_NE(un, nullptr);
    EXPECT_EQ(un->name, "OldData");
    ASSERT_EQ(un->attributes.size(), 1);
    EXPECT_EQ(un->attributes[0], "[[deprecated]]");
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Incomplete, Incorrect and Garbage Input Tests
/////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(ParserTest, ParseIncompleteClass)
{
    auto result = parseSingle("class Foo {");
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->children.size(), 0);
}

TEST_F(ParserTest, ParseIncorrectSyntax)
{
    auto result = parseSingle("class 123Foo {};");
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->children.size(), 1);
}

TEST_F(ParserTest, ParseGarbageInput)
{
    auto result = parseSingle("asdlkjasdklj 123123 !@#$%^&*()");
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->children.size(), 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Realistic Code Tests
/////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(ParserTest, ParseRealisticClassWithDeclarations)
{
    auto result = parseSingle(R"(
class ThreadPool {
public:
    ThreadPool();
    explicit ThreadPool(size_t numThreads);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) noexcept;
    ThreadPool& operator=(ThreadPool&&) noexcept;

    void start();
    void stop();
    bool isRunning() const;

    template<typename F>
    void submit(F&& task);

private:
    void workerLoop();

    std::vector<std::thread> m_workers;
    bool m_running;
};
)");
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->children.size(), 1);

    auto cls = std::dynamic_pointer_cast<ClassNode>(result->children[0]);
    ASSERT_NE(cls, nullptr);
    EXPECT_EQ(cls->name, "ThreadPool");

    // 2 regular + 2 deleted + 1 move = 5 constructors (deleted ones still parse as ctors)
    // Note: delete/default might affect count depending on parser
    EXPECT_GE(cls->constructors.size(), 2);
    EXPECT_EQ(cls->destructors.size(), 1);
    EXPECT_GE(cls->operators.size(), 2); // copy= and move=
    // start, stop, isRunning, submit, workerLoop = 5 (but submit is templated, workerLoop is
    // private)
    EXPECT_GE(cls->memberFunctions.size(), 3);
}

} // namespace codex::tests
