#include <gtest/gtest.h>

#include <codex/preprocessor.hpp>
#include <codex/source.hpp>

#include "test_utils.hpp"

namespace codex::tests
{

class PreprocessorTest : public ::testing::Test
{
   protected:
    void SetUp() override {}
    void TearDown() override {}
};

//--------------------------------------------------------------------------------------------------
// Basic Macro Expansion Tests
//--------------------------------------------------------------------------------------------------

TEST_F(PreprocessorTest, SimpleMacroExpansion)
{
    auto src = makeSource("int x = FOO;");
    std::vector<std::shared_ptr<Source>> sources = {src};

    Preprocessor pp;
    pp.expand(src, std::unordered_map<std::string, std::string>{{"FOO", "42"}});

    EXPECT_EQ(src->content, "int x = 42;");
}

TEST_F(PreprocessorTest, MacroWordBoundary)
{
    auto src = makeSource("int FOOBAR = 1; int FOO = 2;");

    Preprocessor pp;
    pp.expand(src, std::unordered_map<std::string, std::string>{{"FOO", "42"}});

    // FOO should be replaced but FOOBAR should not
    EXPECT_EQ(src->content, "int FOOBAR = 1; int 42 = 2;");
}

TEST_F(PreprocessorTest, SkipDefineLineItself)
{
    auto src = makeSource("#define FOO 42\nint x = FOO;");

    Preprocessor pp;
    pp.expand(src, std::unordered_map<std::string, std::string>{{"FOO", "99"}});

    // The #define line should not have FOO replaced
    EXPECT_TRUE(src->content.find("#define FOO 42") != std::string::npos);
}

TEST_F(PreprocessorTest, MultipleMacros)
{
    auto src = makeSource("int sum = A + B;");
    std::vector<std::shared_ptr<Source>> sources = {src};

    Preprocessor pp;
    pp.expand(src, std::unordered_map<std::string, std::string>{{"A", "10"}, {"B", "20"}});

    EXPECT_EQ(src->content, "int sum = 10 + 20;");
}

TEST_F(PreprocessorTest, EmptySource)
{
    auto src = makeSource("");
    std::vector<std::shared_ptr<Source>> sources = {src};

    Preprocessor pp;
    pp.expand(src, std::unordered_map<std::string, std::string>{{"FOO", "42"}});

    EXPECT_EQ(src->content, "");
}

//--------------------------------------------------------------------------------------------------
// Conditional Directive Tests (#if, #ifdef, #ifndef, #else, #endif)
//--------------------------------------------------------------------------------------------------

TEST_F(PreprocessorTest, IfZeroDisablesBlock)
{
    auto src = makeSource(R"(
#if 0
int disabled = 1;
#endif
int enabled = 2;
)");

    Preprocessor pp;
    pp.expand(src);

    EXPECT_TRUE(src->content.find("[disabled] int disabled") != std::string::npos);
    EXPECT_TRUE(src->content.find("int enabled = 2;") != std::string::npos);
}

TEST_F(PreprocessorTest, IfOneEnablesBlock)
{
    auto src = makeSource(R"(
#if 1
int enabled = 1;
#endif
)");
    std::vector<std::shared_ptr<Source>> sources = {src};

    Preprocessor pp;
    pp.expand(src);

    // The line should NOT be marked as disabled
    EXPECT_TRUE(src->content.find("[disabled]") == std::string::npos ||
                src->content.find("int enabled = 1;") != std::string::npos);
}

TEST_F(PreprocessorTest, IfdefWithDefinedMacro)
{
    auto src = makeSource(R"(
#ifdef DEBUG
int debug_code = 1;
#endif
)");
    std::vector<std::shared_ptr<Source>> sources = {src};

    Preprocessor pp;
    pp.expand(src, std::unordered_map<std::string, std::string>{},
              std::unordered_set<std::string>{"DEBUG"});

    // DEBUG is defined, so block should be enabled
    EXPECT_TRUE(src->content.find("[disabled] int debug_code") == std::string::npos);
}

TEST_F(PreprocessorTest, IfdefWithUndefinedMacro)
{
    auto src = makeSource(R"(
#ifdef UNDEFINED_MACRO
int disabled = 1;
#endif
)");

    Preprocessor pp;
    pp.expand(src);

    // UNDEFINED_MACRO is not defined, so block should be disabled
    EXPECT_TRUE(src->content.find("[disabled] int disabled") != std::string::npos);
}

TEST_F(PreprocessorTest, IfndefWithUndefinedMacro)
{
    auto src = makeSource(R"(
#ifndef UNDEFINED_MACRO
int enabled = 1;
#endif
)");

    Preprocessor pp;
    pp.expand(src);

    // Block should be enabled since macro is NOT defined
    EXPECT_TRUE(src->content.find("[disabled] int enabled") == std::string::npos);
}

TEST_F(PreprocessorTest, IfndefWithDefinedMacro)
{
    auto src = makeSource(R"(
#ifndef DEFINED_MACRO
int disabled = 1;
#endif
)");

    Preprocessor pp;
    pp.expand(src, {}, {"DEFINED_MACRO"});

    // Block should be disabled since macro IS defined
    EXPECT_TRUE(src->content.find("[disabled] int disabled") != std::string::npos);
}

TEST_F(PreprocessorTest, IfElse)
{
    auto src = makeSource(R"(
#if 0
int disabled = 1;
#else
int enabled = 2;
#endif
)");

    Preprocessor pp;
    pp.expand(src);

    EXPECT_TRUE(src->content.find("[disabled] int disabled") != std::string::npos);
    EXPECT_TRUE(src->content.find("[disabled] int enabled") == std::string::npos);
}

TEST_F(PreprocessorTest, NestedConditionals)
{
    auto src = makeSource(R"(
#if 1
  #if 0
  int inner_disabled = 1;
  #endif
  int outer_enabled = 2;
#endif
)");

    Preprocessor pp;
    pp.expand(src);

    EXPECT_TRUE(src->content.find("[disabled] int inner_disabled") != std::string::npos);
    EXPECT_TRUE(src->content.find("[disabled] int outer_enabled") == std::string::npos);
}

TEST_F(PreprocessorTest, DefinedOperator)
{
    auto src = makeSource(R"(
#if defined(MY_MACRO)
int enabled = 1;
#endif
)");

    Preprocessor pp;
    pp.expand(src, {}, {"MY_MACRO"});

    EXPECT_TRUE(src->content.find("[disabled] int enabled") == std::string::npos);
}

TEST_F(PreprocessorTest, DefinedOperatorNegated)
{
    auto src = makeSource(R"(
#if !defined(MY_MACRO)
int enabled = 1;
#endif
)");

    Preprocessor pp; // MY_MACRO not defined
    pp.expand(src);

    EXPECT_TRUE(src->content.find("[disabled] int enabled") == std::string::npos);
}

TEST_F(PreprocessorTest, BooleanAndOperator)
{
    auto src = makeSource(R"(
#if defined(A) && defined(B)
int both_defined = 1;
#endif
)");
    std::vector<std::shared_ptr<Source>> sources = {src};

    // Only A is defined
    Preprocessor pp;
    pp.expand(src, {}, {"A"});

    EXPECT_TRUE(src->content.find("[disabled] int both_defined") != std::string::npos);
}

TEST_F(PreprocessorTest, BooleanOrOperator)
{
    auto src = makeSource(R"(
#if defined(A) || defined(B)
int either_defined = 1;
#endif
)");
    std::vector<std::shared_ptr<Source>> sources = {src};

    // Only A is defined
    Preprocessor pp;
    pp.expand(src, {}, {"A"});

    // Block should be enabled (A || B where A=true)
    EXPECT_TRUE(src->content.find("[disabled] int either_defined") == std::string::npos);
}

TEST_F(PreprocessorTest, InlineDefineAddsToDefinedMacros)
{
    auto src = makeSource(R"(
#define MY_LOCAL_MACRO
#ifdef MY_LOCAL_MACRO
int enabled = 1;
#endif
)");

    Preprocessor pp;
    pp.expand(src);

    // The #define should add MY_LOCAL_MACRO to defined macros
    EXPECT_TRUE(src->content.find("[disabled] int enabled") == std::string::npos);
}

TEST_F(PreprocessorTest, UndefRemovesFromDefinedMacros)
{
    auto src = makeSource(R"(
#define MY_MACRO
#undef MY_MACRO
#ifdef MY_MACRO
int disabled = 1;
#endif
)");

    Preprocessor pp;
    pp.expand(src);

    // After #undef, MY_MACRO should not be defined
    EXPECT_TRUE(src->content.find("[disabled] int disabled") != std::string::npos);
}

} // namespace codex::tests
