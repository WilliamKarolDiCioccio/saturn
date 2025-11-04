#include <gtest/gtest.h>
#include <variant>
#include <functional>
#include <string>

#include <pieces/core/result.hpp>

using namespace pieces;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Helpers for Testing
////////////////////////////////////////////////////////////////////////////////////////////////////

Result<int, std::string> multiplyByTwo(int v) { return Ok<int, std::string>(v * 2); }

Result<int, std::string> failWithMessage(const std::string& msg)
{
    return Err<int, std::string>(std::string("Error: ") + msg);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Tests
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(UnwrapRefTest, PlainType)
{
    int x = 42;
    int& ref = UnwrapRef<int>::get(x);

    EXPECT_EQ(ref, 42);

    ref = 100;

    EXPECT_EQ(x, 100);
}

TEST(UnwrapRefTest, ReferenceWrapper)
{
    int x = 7;
    std::reference_wrapper<int> rw(x);
    int& ref = UnwrapRef<std::reference_wrapper<int>>::get(rw);

    EXPECT_EQ(ref, 7);

    ref = 20;

    EXPECT_EQ(x, 20);
}

TEST(ResultTest, OkAndErrStates)
{
    auto okRes = Result<int, std::string>::Ok(123);
    EXPECT_TRUE(okRes.isOk());
    EXPECT_FALSE(okRes.isErr());
    EXPECT_EQ(okRes.unwrap(), 123);

    auto errRes = Result<int, std::string>::Err("failure");
    EXPECT_TRUE(errRes.isErr());
    EXPECT_FALSE(errRes.isOk());
    EXPECT_EQ(errRes.error(), "failure");
}

TEST(ResultTest, UnwrapThrowsOnErr)
{
    auto errRes = Result<int, std::string>::Err("oops");
    EXPECT_THROW(errRes.unwrap(), std::runtime_error);
}

TEST(ResultTest, ErrorThrowsOnOk)
{
    auto okRes = Result<int, std::string>::Ok(10);
    EXPECT_THROW(okRes.error(), std::runtime_error);
}

TEST(ResultTest, AndThenChainsOnOk)
{
    auto okRes = Ok<int, std::string>(5);
    auto result = okRes.andThen(multiplyByTwo);

    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(result.unwrap(), 10);
}

TEST(ResultTest, AndThenShortCircuitsOnErr)
{
    auto errRes = Err<int, std::string>("bad");
    auto chained = errRes.andThen(multiplyByTwo);

    EXPECT_TRUE(chained.isErr());
    EXPECT_EQ(chained.error(), "bad");
}

TEST(ResultTest, OrElseChainsOnErr)
{
    auto errRes = Err<int, std::string>("problem");
    auto recovered = errRes.orElse(failWithMessage);

    EXPECT_TRUE(recovered.isErr());
    EXPECT_EQ(recovered.error(), "Error: problem");
}

TEST(ResultTest, OrElsePassesThroughOnOk)
{
    auto okRes = Ok<int, std::string>(7);
    auto result = okRes.orElse(failWithMessage);

    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(result.unwrap(), 7);
}

// Test free functions Ok and Err
TEST(FreeFunctionsTest, OkFunctionCreatesOk)
{
    auto res = Ok<std::string, int>("hello");

    EXPECT_TRUE(res.isOk());
    EXPECT_EQ(res.unwrap(), "hello");
}

TEST(FreeFunctionsTest, ErrFunctionCreatesErr)
{
    auto res = Err<std::string, int>(42);
    EXPECT_TRUE(res.isErr());
    EXPECT_EQ(res.error(), 42);
}

// Test RefResult and OkRef / ErrRef
TEST(RefResultTest, OkRefContainsReference)
{
    int value = 55;
    auto refRes = OkRef<int, std::string>(value);

    EXPECT_TRUE(refRes.isOk());

    auto& rw = refRes.unwrap();
    rw = 99;

    EXPECT_EQ(value, 99);
}

// Test chaining operations
TEST(ChainingTest, MultipleAndThenChaining)
{
    auto result = Ok<int, std::string>(2)
                      .andThen(multiplyByTwo)                                      // 4
                      .andThen(multiplyByTwo)                                      // 8
                      .andThen([](int v) { return Ok<int, std::string>(v + 1); }); // 9

    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(result.unwrap(), 9);
}

TEST(ChainingTest, ShortCircuitOnFirstErr)
{
    auto result = Err<int, std::string>("fail1")
                      .andThen(multiplyByTwo)
                      .andThen([](int v) { return Err<int, std::string>("fail2"); });

    EXPECT_TRUE(result.isErr());
    EXPECT_EQ(result.error(), "fail1");
}

TEST(ChainingTest, MultipleOrElseChaining)
{
    auto result = Err<int, std::string>("e1")
                      .orElse([](const std::string& e) { return Err<int, std::string>(e + ":e2"); })
                      .orElse([](const std::string& e) { return Ok<int, std::string>(0); });

    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(result.unwrap(), 0);
}

TEST(ChainingTest, OrElsePassThruOnOkFirst)
{
    auto result = Ok<int, std::string>(3)
                      .orElse([](const std::string&) { return Err<int, std::string>("ignored"); })
                      .andThen(multiplyByTwo);

    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(result.unwrap(), 6);
}

TEST(ChainingTest, AndThenThenOrElse)
{
    auto result =
        Ok<int, std::string>(5)
            .andThen([](int v) { return Err<int, std::string>("err:" + std::to_string(v)); })
            .orElse([](const std::string& e) { return Ok<int, std::string>(-1); });

    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(result.unwrap(), -1);
}
