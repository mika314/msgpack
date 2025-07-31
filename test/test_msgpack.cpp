#include "../msgpack-ser.hpp"
#include <catch2/catch.hpp>
#include <ser/macro.hpp>
#include <sstream>

struct Test
{
  SER_PROPS(one, two)
  int one;
  std::string two;
};

struct Test2
{
  SER_PROPS(a, b, c, d, e, f)
  int a;
  unsigned int b;
  int64_t c;
  uint64_t d;
  float e;
  double f;
};

struct Test3
{
  SER_PROPS(vec, map, variant)
  std::vector<int> vec;
  std::map<std::string, Test> map;
  std::variant<int, std::string, Test> variant;
};

struct TestMismatched
{
  SER_PROPS(one, two)
  float one;
  bool two;
};

TEST_CASE("Msgpack serialization and deserialization", "[msgpack-ser]")
{
  SECTION("Simple struct")
  {
    auto ss = std::ostringstream{};
    Test test;
    test.one = 420;
    test.two = "hello again";
    msgpackSer(ss, test);

    auto iss = std::istringstream{ss.str()};
    Test test2;
    msgpackDeser(iss, test2);

    REQUIRE(test.one == test2.one);
    REQUIRE(test.two == test2.two);
  }

  SECTION("Integer and floating point types")
  {
    auto ss = std::ostringstream{};
    Test2 test;
    test.a = -1;
    test.b = 1;
    test.c = -1234567890;
    test.d = 1234567890;
    test.e = 3.14f;
    test.f = 3.141592653589793;
    msgpackSer(ss, test);

    auto iss = std::istringstream{ss.str()};
    Test2 test2;
    msgpackDeser(iss, test2);

    REQUIRE(test.a == test2.a);
    REQUIRE(test.b == test2.b);
    REQUIRE(test.c == test2.c);
    REQUIRE(test.d == test2.d);
    REQUIRE(test.e == Approx(test2.e));
    REQUIRE(test.f == Approx(test2.f));
  }

  SECTION("Nested structs, collections, and variants")
  {
    auto ss = std::ostringstream{};
    Test3 test;
    test.vec = {1, 2, 3};
    test.map["a"] = {1, "one"};
    test.map["b"] = {2, "two"};
    test.variant = "hello variant";
    msgpackSer(ss, test);

    auto iss = std::istringstream{ss.str()};
    Test3 test2;
    msgpackDeser(iss, test2);

    REQUIRE(test.vec == test2.vec);
    REQUIRE(test.map.size() == test2.map.size());
    REQUIRE(test.map["a"].one == test2.map["a"].one);
    REQUIRE(test.map["a"].two == test2.map["a"].two);
    REQUIRE(test.map["b"].one == test2.map["b"].one);
    REQUIRE(test.map["b"].two == test2.map["b"].two);
    REQUIRE(std::get<std::string>(test.variant) == std::get<std::string>(test2.variant));

    ss.str("");
    test.variant = 123;
    msgpackSer(ss, test);
    iss.clear();
    iss.str(ss.str());
    msgpackDeser(iss, test2);
    REQUIRE(std::get<int>(test.variant) == std::get<int>(test2.variant));

    ss.str("");
    test.variant = Test{1, "test"};
    msgpackSer(ss, test);
    iss.clear();
    iss.str(ss.str());
    msgpackDeser(iss, test2);
    REQUIRE(std::get<Test>(test.variant).one == std::get<Test>(test2.variant).one);
    REQUIRE(std::get<Test>(test.variant).two == std::get<Test>(test2.variant).two);
  }

  SECTION("Malformed input")
  {
    std::string malformed_input = "\x81\xa1z";
    auto iss = std::istringstream{malformed_input};
    Test test;
    REQUIRE_THROWS_AS(msgpackDeser(iss, test), msgpack::ParsingError);
  }

  SECTION("Mismatched types")
  {
    auto ss = std::ostringstream{};
    Test test;
    test.one = 420;
    test.two = "hello again";
    msgpackSer(ss, test);

    auto iss = std::istringstream{ss.str()};
    TestMismatched test2;
    REQUIRE_THROWS_WITH(msgpackDeser(iss, test2), "Type mismatch. Expected float, got uint64_t");
  }
}
