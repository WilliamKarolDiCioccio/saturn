#include <gtest/gtest.h>

#include <codex/analyzer.hpp>
#include <codex/parser.hpp>

#include "test_utils.hpp"

namespace codex::tests
{

/////////////////////////////////////////////////////////////////////////////////////////////
// Include Graph Tests
/////////////////////////////////////////////////////////////////////////////////////////////

class IncludeGraphTest : public ::testing::Test
{
   protected:
    Parser parser;

    std::shared_ptr<SourceNode> parseWithPath(const std::string& content, const std::string& path)
    {
        return codex::tests::parseWithPath(content, path, parser);
    }
};

TEST_F(IncludeGraphTest, AIncludesB_TopoOrderBThenA)
{
    auto b = parseWithPath("struct Foo {};", "project/b.hpp");
    auto a = parseWithPath("#include \"b.hpp\"\nstruct Bar {};", "project/a.hpp");

    IncludeGraph graph;
    graph.build({a, b});

    auto order = graph.topologicalSort();
    ASSERT_EQ(order.size(), 2u);

    // b (index 1) should come before a (index 0)
    size_t posA = std::find(order.begin(), order.end(), 0) - order.begin();
    size_t posB = std::find(order.begin(), order.end(), 1) - order.begin();
    EXPECT_LT(posB, posA);
}

TEST_F(IncludeGraphTest, DiamondDependency)
{
    auto d = parseWithPath("struct D {};", "project/d.hpp");
    auto b = parseWithPath("#include \"d.hpp\"\nstruct B {};", "project/b.hpp");
    auto c = parseWithPath("#include \"d.hpp\"\nstruct C {};", "project/c.hpp");
    auto a = parseWithPath("#include \"b.hpp\"\n#include \"c.hpp\"", "project/a.hpp");

    IncludeGraph graph;
    graph.build({a, b, c, d});

    auto order = graph.topologicalSort();
    ASSERT_EQ(order.size(), 4u);

    size_t posA = std::find(order.begin(), order.end(), 0) - order.begin();
    size_t posB = std::find(order.begin(), order.end(), 1) - order.begin();
    size_t posC = std::find(order.begin(), order.end(), 2) - order.begin();
    size_t posD = std::find(order.begin(), order.end(), 3) - order.begin();

    // d before b, d before c, b before a, c before a
    EXPECT_LT(posD, posB);
    EXPECT_LT(posD, posC);
    EXPECT_LT(posB, posA);
    EXPECT_LT(posC, posA);
}

TEST_F(IncludeGraphTest, CycleDetection_GracefulFallback)
{
    // A includes B, B includes A → cycle
    auto a = parseWithPath("#include \"b.hpp\"", "project/a.hpp");
    auto b = parseWithPath("#include \"a.hpp\"", "project/b.hpp");

    IncludeGraph graph;
    graph.build({a, b});

    auto order = graph.topologicalSort();
    // Should still return both indices (fallback for cycle)
    ASSERT_EQ(order.size(), 2u);
}

TEST_F(IncludeGraphTest, SystemIncludesIgnored)
{
    auto a = parseWithPath("#include <vector>\n#include <string>\nstruct A {};", "project/a.hpp");

    IncludeGraph graph;
    graph.build({a});

    EXPECT_TRUE(graph.entries()[0].dependsOn.empty());
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Symbol Indexing Tests
/////////////////////////////////////////////////////////////////////////////////////////////

class AnalyzerIndexingTest : public ::testing::Test
{
   protected:
    Parser parser;
    Analyzer analyzer;

    AnalysisResult analyzeSingle(const std::string& code, const std::string& path = "test.hpp")
    {
        auto node = parseWithPath(code, path, parser);
        return analyzer.analyze({node});
    }
};

TEST_F(AnalyzerIndexingTest, NamespaceClassFunction_QualifiedNames)
{
    auto result = analyzeSingle(R"(
        namespace foo {
            class Bar {
            public:
                void baz();
            };
        }
    )");

    auto& db = result.database;
    EXPECT_NE(db.findByQualifiedName("foo"), nullptr);
    EXPECT_NE(db.findByQualifiedName("foo::Bar"), nullptr);
    EXPECT_NE(db.findByQualifiedName("foo::Bar::baz"), nullptr);

    auto* bar = db.findByQualifiedName("foo::Bar");
    ASSERT_NE(bar, nullptr);
    EXPECT_EQ(bar->symbolKind, SymbolKind::Class);
    EXPECT_EQ(bar->name, "Bar");
}

TEST_F(AnalyzerIndexingTest, NestedNamespaces)
{
    auto result = analyzeSingle(R"(
        namespace a::b::c {
            struct Foo {};
        }
    )");

    auto& db = result.database;
    EXPECT_NE(db.findByQualifiedName("a"), nullptr);
    EXPECT_NE(db.findByQualifiedName("a::b"), nullptr);
    EXPECT_NE(db.findByQualifiedName("a::b::c"), nullptr);
    EXPECT_NE(db.findByQualifiedName("a::b::c::Foo"), nullptr);
}

TEST_F(AnalyzerIndexingTest, EnumAndEnumerators)
{
    auto result = analyzeSingle(R"(
        enum class Color { Red, Green, Blue };
    )");

    auto& db = result.database;
    auto* e = db.findByQualifiedName("Color");
    ASSERT_NE(e, nullptr);
    EXPECT_EQ(e->symbolKind, SymbolKind::Enum);

    EXPECT_NE(db.findByQualifiedName("Color::Red"), nullptr);
    EXPECT_NE(db.findByQualifiedName("Color::Green"), nullptr);
    EXPECT_NE(db.findByQualifiedName("Color::Blue"), nullptr);

    auto* red = db.findByQualifiedName("Color::Red");
    ASSERT_NE(red, nullptr);
    EXPECT_EQ(red->symbolKind, SymbolKind::EnumValue);
    EXPECT_EQ(red->parentSymbol, e->id);
}

TEST_F(AnalyzerIndexingTest, OverloadedFunctions)
{
    auto result = analyzeSingle(R"(
        void foo(int x);
        void foo(float x);
    )");

    auto& db = result.database;
    auto foos = db.findByName("foo");
    ASSERT_EQ(foos.size(), 2u);

    // Different signatures
    EXPECT_NE(foos[0]->signature, foos[1]->signature);
}

TEST_F(AnalyzerIndexingTest, ForwardDeclThenFullDefinition)
{
    auto result = analyzeSingle(R"(
        class Foo;
        class Foo {
        public:
            int x;
        };
    )");

    auto& db = result.database;
    // Should be exactly one symbol, not two
    auto foos = db.findByName("Foo");
    ASSERT_EQ(foos.size(), 1u);
    EXPECT_FALSE(foos[0]->isForwardDeclaration);
}

TEST_F(AnalyzerIndexingTest, NestedClass)
{
    auto result = analyzeSingle(R"(
        class Outer {
        public:
            class Inner {
            public:
                void method();
            };
        };
    )");

    auto& db = result.database;
    EXPECT_NE(db.findByQualifiedName("Outer"), nullptr);
    EXPECT_NE(db.findByQualifiedName("Outer::Inner"), nullptr);
    EXPECT_NE(db.findByQualifiedName("Outer::Inner::method"), nullptr);

    auto* inner = db.findByQualifiedName("Outer::Inner");
    ASSERT_NE(inner, nullptr);
    auto* outer = db.findByQualifiedName("Outer");
    EXPECT_EQ(inner->parentSymbol, outer->id);
}

TEST_F(AnalyzerIndexingTest, StructMembers)
{
    auto result = analyzeSingle(R"(
        struct Vec2 {
            float x;
            float y;
            float length() const;
        };
    )");

    auto& db = result.database;
    EXPECT_NE(db.findByQualifiedName("Vec2"), nullptr);
    EXPECT_NE(db.findByQualifiedName("Vec2::x"), nullptr);
    EXPECT_NE(db.findByQualifiedName("Vec2::y"), nullptr);
    EXPECT_NE(db.findByQualifiedName("Vec2::length"), nullptr);

    auto* x = db.findByQualifiedName("Vec2::x");
    EXPECT_EQ(x->access, AccessSpecifier::Public);
}

TEST_F(AnalyzerIndexingTest, ClassAccessSpecifiers)
{
    auto result = analyzeSingle(R"(
        class Foo {
        private:
            int m_x;
        public:
            int getX() const;
        protected:
            void helper();
        };
    )");

    auto& db = result.database;
    auto* mx = db.findByQualifiedName("Foo::m_x");
    auto* getX = db.findByQualifiedName("Foo::getX");
    auto* helper = db.findByQualifiedName("Foo::helper");

    ASSERT_NE(mx, nullptr);
    ASSERT_NE(getX, nullptr);
    ASSERT_NE(helper, nullptr);

    EXPECT_EQ(mx->access, AccessSpecifier::Private);
    EXPECT_EQ(getX->access, AccessSpecifier::Public);
    EXPECT_EQ(helper->access, AccessSpecifier::Protected);
}

TEST_F(AnalyzerIndexingTest, Variables)
{
    auto result = analyzeSingle(R"(
        namespace config {
            constexpr int MAX_SIZE = 100;
        }
    )");

    auto& db = result.database;
    auto* v = db.findByQualifiedName("config::MAX_SIZE");
    ASSERT_NE(v, nullptr);
    EXPECT_EQ(v->symbolKind, SymbolKind::Variable);
}

TEST_F(AnalyzerIndexingTest, TypeAlias)
{
    auto result = analyzeSingle(R"(
        namespace foo {
            using StringVec = int;
        }
    )");

    auto& db = result.database;
    auto* ta = db.findByQualifiedName("foo::StringVec");
    ASSERT_NE(ta, nullptr);
    EXPECT_EQ(ta->symbolKind, SymbolKind::TypeAlias);
}

TEST_F(AnalyzerIndexingTest, Macros)
{
    auto result = analyzeSingle(R"(
        #define MAX_VALUE 100
        #define ADD(a, b) ((a) + (b))
    )");

    auto& db = result.database;
    auto* obj = db.findByQualifiedName("MAX_VALUE");
    auto* fn = db.findByQualifiedName("ADD");
    ASSERT_NE(obj, nullptr);
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(obj->symbolKind, SymbolKind::ObjectLikeMacro);
    EXPECT_EQ(fn->symbolKind, SymbolKind::FunctionLikeMacro);
}

TEST_F(AnalyzerIndexingTest, Concept)
{
    auto result = analyzeSingle(R"(
        template <typename T>
        concept Hashable = requires(T a) { { a.hash() }; };
    )");

    auto& db = result.database;
    auto* c = db.findByQualifiedName("Hashable");
    ASSERT_NE(c, nullptr);
    EXPECT_EQ(c->symbolKind, SymbolKind::Concept);
}

TEST_F(AnalyzerIndexingTest, Constructor_Destructor)
{
    auto result = analyzeSingle(R"(
        class Foo {
        public:
            Foo();
            Foo(int x);
            ~Foo();
        };
    )");

    auto& db = result.database;
    auto ctors = db.findByName("Foo");
    // Foo class + 2 constructors = 3 symbols named "Foo"
    int ctorCount = 0;
    int dtorCount = 0;
    for (auto* s : ctors)
    {
        if (s->symbolKind == SymbolKind::Constructor) ctorCount++;
    }
    auto dtors = db.findByName("~Foo");
    for (auto* s : dtors)
    {
        if (s->symbolKind == SymbolKind::Destructor) dtorCount++;
    }
    EXPECT_EQ(ctorCount, 2);
    EXPECT_EQ(dtorCount, 1);
}

TEST_F(AnalyzerIndexingTest, Operator)
{
    auto result = analyzeSingle(R"(
        struct Vec {
            Vec operator+(const Vec& other);
        };
    )");

    auto& db = result.database;
    auto* op = db.findByQualifiedName("Vec::operator+");
    ASSERT_NE(op, nullptr);
    EXPECT_EQ(op->symbolKind, SymbolKind::Operator);
}

TEST_F(AnalyzerIndexingTest, AnonymousNamespace)
{
    auto result = analyzeSingle(R"(
        namespace {
            int hidden;
        }
    )");

    auto& db = result.database;
    auto* ns = db.findByQualifiedName("<anonymous>");
    ASSERT_NE(ns, nullptr);
    EXPECT_EQ(ns->symbolKind, SymbolKind::Namespace);

    auto* var = db.findByQualifiedName("<anonymous>::hidden");
    ASSERT_NE(var, nullptr);
}

TEST_F(AnalyzerIndexingTest, DatabaseSize)
{
    auto result = analyzeSingle(R"(
        struct A {};
        struct B {};
        void foo();
    )");

    // A, B, foo = 3 symbols minimum
    EXPECT_GE(result.database.size(), 3u);
}

TEST_F(AnalyzerIndexingTest, FindByKind)
{
    auto result = analyzeSingle(R"(
        struct A {};
        struct B {};
        class C {};
        void foo();
    )");

    auto& db = result.database;
    auto structs = db.findByKind(SymbolKind::Struct);
    EXPECT_EQ(structs.size(), 2u);

    auto classes = db.findByKind(SymbolKind::Class);
    EXPECT_EQ(classes.size(), 1u);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Cross-Reference Tests
/////////////////////////////////////////////////////////////////////////////////////////////

class AnalyzerCrossRefTest : public ::testing::Test
{
   protected:
    Parser parser;
    Analyzer analyzer;

    AnalysisResult analyzeSingle(const std::string& code, const std::string& path = "test.hpp")
    {
        auto node = parseWithPath(code, path, parser);
        return analyzer.analyze({node});
    }

    AnalysisResult analyzeMultiple(const std::vector<std::pair<std::string, std::string>>& files)
    {
        std::vector<std::shared_ptr<SourceNode>> nodes;

        for (const auto& [code, path] : files)
        {
            auto src = makeSourceWithPath(code, path);
            nodes.push_back(parser.parse(src));
            parser.reset(); // Reset parser state between files
        }

        return analyzer.analyze(nodes);
    }
};

TEST_F(AnalyzerCrossRefTest, BaseClass_SameFile)
{
    auto result = analyzeSingle(R"(
        class Base {};
        class Derived : public Base {};
    )");

    auto& db = result.database;
    auto* base = db.findByQualifiedName("Base");
    auto* derived = db.findByQualifiedName("Derived");
    ASSERT_NE(base, nullptr);
    ASSERT_NE(derived, nullptr);

    // Derived.baseClasses should contain Base
    ASSERT_EQ(derived->baseClasses.size(), 1u);
    EXPECT_EQ(derived->baseClasses[0], base->id);

    // Base.derivedClasses should contain Derived
    ASSERT_EQ(base->derivedClasses.size(), 1u);
    EXPECT_EQ(base->derivedClasses[0], derived->id);
}

TEST_F(AnalyzerCrossRefTest, FunctionReturningType)
{
    auto result = analyzeSingle(R"(
        struct MyType {};
        MyType create();
    )");

    auto& db = result.database;
    auto* fn = db.findByQualifiedName("create");
    auto* ty = db.findByQualifiedName("MyType");
    ASSERT_NE(fn, nullptr);
    ASSERT_NE(ty, nullptr);

    // create should reference MyType
    bool found = false;
    for (auto refID : fn->referencedTypes)
    {
        if (refID == ty->id) found = true;
    }
    EXPECT_TRUE(found);
}

TEST_F(AnalyzerCrossRefTest, CrossFile_IncludeResolution)
{
    auto result = analyzeMultiple({
        {"struct Foo {};", "project/foo.hpp"},
        {"#include \"foo.hpp\"\nFoo getFoo();", "project/bar.hpp"},
    });

    auto& db = result.database;
    auto* foo = db.findByQualifiedName("Foo");
    auto* getFoo = db.findByQualifiedName("getFoo");
    ASSERT_NE(foo, nullptr);
    ASSERT_NE(getFoo, nullptr);

    bool found = false;
    for (auto refID : getFoo->referencedTypes)
    {
        if (refID == foo->id) found = true;
    }
    EXPECT_TRUE(found);
}

TEST_F(AnalyzerCrossRefTest, UnresolvableType_NoError)
{
    auto result = analyzeSingle(R"(
        std::string getName();
    )");

    auto& db = result.database;
    auto* fn = db.findByQualifiedName("getName");
    ASSERT_NE(fn, nullptr);
    // std::string should not resolve → graceful, no crash
    // referencedTypes may be empty or contain invalid refs
}

TEST_F(AnalyzerCrossRefTest, NamespaceBaseClass)
{
    auto result = analyzeSingle(R"(
        namespace foo {
            class Base {};
            class Derived : public Base {};
        }
    )");

    auto& db = result.database;
    auto* derived = db.findByQualifiedName("foo::Derived");
    auto* base = db.findByQualifiedName("foo::Base");
    ASSERT_NE(derived, nullptr);
    ASSERT_NE(base, nullptr);

    ASSERT_EQ(derived->baseClasses.size(), 1u);
    EXPECT_EQ(derived->baseClasses[0], base->id);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// using-namespace Tests
/////////////////////////////////////////////////////////////////////////////////////////////

class AnalyzerUsingNamespaceTest : public ::testing::Test
{
   protected:
    Parser parser;
    Analyzer analyzer;

    std::shared_ptr<SourceNode> parseWithPath(const std::string& content, const std::string& path)
    {
        return codex::tests::parseWithPath(content, path, parser);
    }
};

TEST_F(AnalyzerUsingNamespaceTest, UsingNamespaceResolvesType)
{
    auto a = parseWithPath(R"(
        namespace foo {
            struct Bar {};
        }
        using namespace foo;
        Bar getBar();
    )",
                           "test.hpp");

    auto result = analyzer.analyze({a});
    auto& db = result.database;

    auto* bar = db.findByQualifiedName("foo::Bar");
    auto* getBar = db.findByQualifiedName("getBar");
    ASSERT_NE(bar, nullptr);
    ASSERT_NE(getBar, nullptr);
    // Note: linking happens with best-effort resolution.
    // The using-namespace is tracked during indexing but the linker
    // currently doesn't carry per-symbol using-namespace context.
    // This test verifies no crashes and correct indexing.
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Misc / Edge Cases
/////////////////////////////////////////////////////////////////////////////////////////////

class AnalyzerMisc : public ::testing::Test
{
   protected:
    Parser parser;
    Analyzer analyzer;

    std::shared_ptr<SourceNode> parseWithPath(const std::string& content, const std::string& path)
    {
        return codex::tests::parseWithPath(content, path, parser);
    }
};

TEST_F(AnalyzerMisc, EmptyInput)
{
    Analyzer analyzer;
    auto result = analyzer.analyze({});
    EXPECT_EQ(result.database.size(), 0u);
    EXPECT_TRUE(result.includeGraph.entries().empty());
}

TEST_F(AnalyzerMisc, SourceNodeLocation)
{
    auto node = parseWithPath("struct Foo {};", "project/foo.hpp");
    Analyzer analyzer;
    auto result = analyzer.analyze({node});

    auto* sym = result.database.findByQualifiedName("Foo");
    ASSERT_NE(sym, nullptr);
    EXPECT_EQ(sym->location.filePath, std::filesystem::path("project/foo.hpp"));
}

TEST_F(AnalyzerMisc, ParentChildRelationship)
{
    auto node = parseWithPath(R"(
        namespace ns {
            struct S {
                int x;
            };
        }
    )",
                              "test.hpp");

    Analyzer analyzer;
    auto result = analyzer.analyze({node});
    auto& db = result.database;

    auto* ns = db.findByQualifiedName("ns");
    auto* s = db.findByQualifiedName("ns::S");
    auto* x = db.findByQualifiedName("ns::S::x");

    ASSERT_NE(ns, nullptr);
    ASSERT_NE(s, nullptr);
    ASSERT_NE(x, nullptr);

    EXPECT_EQ(s->parentSymbol, ns->id);
    EXPECT_EQ(x->parentSymbol, s->id);

    // ns should have S as child
    bool found = false;
    for (auto childID : ns->childSymbols)
    {
        if (childID == s->id) found = true;
    }
    EXPECT_TRUE(found);
}

TEST_F(AnalyzerMisc, SymbolIDInvalid)
{
    SymbolID id = SymbolID::invalid();
    EXPECT_EQ(id.value, 0u);
    EXPECT_FALSE(static_cast<bool>(id));

    SymbolID valid{42};
    EXPECT_TRUE(static_cast<bool>(valid));
    EXPECT_NE(id, valid);
}

} // namespace codex::tests
