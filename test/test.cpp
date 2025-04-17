#include <catch2/catch.hpp>
#include <msgpack/msgpack.hpp>
#include <sstream>

TEST_CASE("Positive and negative fixint", "[msgpack]")
{
  // +42
  {
    const auto buf = std::vector<std::byte>{std::byte{0x2a}};
    const auto b = msgpack::Blob{std::span(buf)};
    REQUIRE(std::holds_alternative<int64_t>(b.val));
    REQUIRE(std::get<int64_t>(b.val) == 42);
  }
  // -5
  {
    const auto buf = std::vector<std::byte>{std::byte{0xfb}};
    const auto b = msgpack::Blob{std::span(buf)};
    REQUIRE(std::holds_alternative<int64_t>(b.val));
    REQUIRE(std::get<int64_t>(b.val) == -5);
  }
}

TEST_CASE("Nil and bool", "[msgpack]")
{
  {
    const auto buf = std::vector<std::byte>{std::byte{0xc0}}; // nil
    const auto b = msgpack::Blob{std::span(buf)};
    REQUIRE(std::holds_alternative<std::nullptr_t>(b.val));
  }
  {
    const auto buf = std::vector<std::byte>{std::byte{0xc2}}; // false
    const auto b = msgpack::Blob{std::span(buf)};
    REQUIRE(std::holds_alternative<bool>(b.val));
    REQUIRE(std::get<bool>(b.val) == false);
  }
  {
    const auto buf = std::vector<std::byte>{std::byte{0xc3}}; // true
    const auto b = msgpack::Blob{std::span(buf)};
    REQUIRE(std::get<bool>(b.val) == true);
  }
}

TEST_CASE("Fixstr and str8/16/32", "[msgpack]")
{
  // fixstr "hello" (len=5 → 0xa0 + 5 = 0xa5)
  {
    const auto s = std::string{"hello"};
    const auto buf = std::vector<std::byte>{
      std::byte{0xa5}, std::byte{'h'}, std::byte{'e'}, std::byte{'l'}, std::byte{'l'}, std::byte{'o'}};
    const auto b = msgpack::Blob{std::span{buf}};
    REQUIRE(std::holds_alternative<std::string_view>(b.val));
    REQUIRE(std::get<std::string_view>(b.val) == s);
  }
  // str8 "world123457890123457890123457890"
  {
    const auto s = std::string{"world123456789012345678901234567"};
    const auto buf = std::vector<std::byte>{
      std::byte{0xd9}, std::byte{0x20}, std::byte{'w'}, std::byte{'o'}, std::byte{'r'}, std::byte{'l'},
      std::byte{'d'},  std::byte{'1'},  std::byte{'2'}, std::byte{'3'}, std::byte{'4'}, std::byte{'5'},
      std::byte{'6'},  std::byte{'7'},  std::byte{'8'}, std::byte{'9'}, std::byte{'0'}, std::byte{'1'},
      std::byte{'2'},  std::byte{'3'},  std::byte{'4'}, std::byte{'5'}, std::byte{'6'}, std::byte{'7'},
      std::byte{'8'},  std::byte{'9'},  std::byte{'0'}, std::byte{'1'}, std::byte{'2'}, std::byte{'3'},
      std::byte{'4'},  std::byte{'5'},  std::byte{'6'}, std::byte{'7'}};
    auto const b = msgpack::Blob{std::span(buf)};
    REQUIRE(std::get<std::string_view>(b.val) == s);
  }
  // str16 with 256 'a's
  {
    std::string s(256, 'a');
    std::vector<std::byte> buf;
    buf.push_back(std::byte{0xda});
    buf.push_back(std::byte{0x01});
    buf.push_back(std::byte{0x00});
    for (auto c : s)
      buf.push_back(static_cast<std::byte>(c));
    const auto b = msgpack::Blob{std::span(buf)};
    REQUIRE(std::get<std::string_view>(b.val) == s);
  }
}

TEST_CASE("Binary (bin8/bin16/bin32)", "[msgpack]")
{
  std::vector<std::byte> data = {std::byte{0xde}, std::byte{0xad}, std::byte{0xbe}, std::byte{0xef}};
  // bin8
  {
    auto buf = std::vector<std::byte>{std::byte{0xC4}, std::byte{4}};
    buf.insert(buf.end(), data.begin(), data.end());
    const auto b = msgpack::Blob{std::span(buf)};
    auto sp = std::get<std::span<const std::byte>>(b.val);
    REQUIRE(sp.size() == 4);
    for (size_t i = 0; i < 4; ++i)
      REQUIRE(sp[i] == data[i]);
  }
  // bin16
  {
    auto buf = std::vector<std::byte>{std::byte{0xc5}, std::byte{0x00}, std::byte{4}};
    buf.insert(buf.end(), data.begin(), data.end());
    const auto b = msgpack::Blob{std::span(buf)};
    const auto sp = std::get<std::span<const std::byte>>(b.val);
    REQUIRE(sp.size() == 4);
  }
}

TEST_CASE("Unsigned and signed ints", "[msgpack]")
{
  // uint16_t 0x1234
  {
    const auto buf = std::vector<std::byte>{std::byte{0xcd}, std::byte{0x12}, std::byte{0x34}};
    const auto b = msgpack::Blob{std::span(buf)};
    REQUIRE(std::holds_alternative<uint64_t>(b.val));
    REQUIRE(std::get<uint64_t>(b.val) == 0x1234);
  }
  // int32_t -1000
  {
    const auto v = int32_t{-1000};
    const auto raw = static_cast<uint32_t>(v);
    const auto buf = std::vector<std::byte>{std::byte{0xd2},
                                            std::byte{static_cast<uint8_t>(raw >> 24)},
                                            std::byte{static_cast<uint8_t>(raw >> 16)},
                                            std::byte{static_cast<uint8_t>(raw >> 8)},
                                            std::byte{static_cast<uint8_t>(raw)}};
    const auto b = msgpack::Blob{std::span(buf)};
    REQUIRE(std::holds_alternative<int64_t>(b.val));
    REQUIRE(std::get<int64_t>(b.val) == -1000);
  }
}

TEST_CASE("Float32 and Float64", "[msgpack]")
{
  // 1.0f → 0x3f800000
  {
    const auto buf = std::vector<std::byte>{
      std::byte{0xca}, std::byte{0x3f}, std::byte{0x80}, std::byte{0x00}, std::byte{0x00}};
    const auto b = msgpack::Blob{std::span(buf)};
    REQUIRE(std::holds_alternative<float>(b.val));
    REQUIRE(std::get<float>(b.val) == Approx(1.0f));
  }
  // 3.141592653589793 → 0x400921fb54442d18
  {
    const auto buf = std::vector<std::byte>{std::byte{0xcb},
                                            std::byte{0x40},
                                            std::byte{0x09},
                                            std::byte{0x21},
                                            std::byte{0xfb},
                                            std::byte{0x54},
                                            std::byte{0x44},
                                            std::byte{0x2d},
                                            std::byte{0x18}};
    const auto b = msgpack::Blob{std::span(buf)};
    REQUIRE(std::holds_alternative<double>(b.val));
    REQUIRE(std::get<double>(b.val) == Approx(3.141592653589793));
  }
}

TEST_CASE("Fixarray and array16", "[msgpack]")
{
  // [1,2,3] → 0x93 0x01 0x02 0x03
  {
    const auto buf = std::vector<std::byte>{std::byte{0x93}, std::byte{1}, std::byte{2}, std::byte{3}};
    const auto b = msgpack::Blob{std::span(buf)};
    const auto &a = std::get<msgpack::Array>(b.val);
    REQUIRE(a.size() == 3);
    REQUIRE(std::get<int64_t>(a[0]) == 1);
    REQUIRE(std::get<int64_t>(a[1]) == 2);
    REQUIRE(std::get<int64_t>(a[2]) == 3);
  }
  // array16 of two 0xff-s
  {
    const auto buf = std::vector<std::byte>{
      std::byte{0xdc}, std::byte{0x00}, std::byte{0x02}, std::byte{0xff}, std::byte{0xff}};
    const auto b = msgpack::Blob{std::span(buf)};
    const auto &a = std::get<msgpack::Array>(b.val);
    REQUIRE(a.size() == 2);
    REQUIRE(std::get<int64_t>(a[0]) == -1);
    REQUIRE(std::get<int64_t>(a[1]) == -1);
  }
}

TEST_CASE("Fixmap and map16", "[msgpack]")
{
  // { "a": 1, "b": 2 } → 0x82 0xa1 'a' 0x01 0xa1 'b' 0x02
  {
    const auto buf = std::vector<std::byte>{std::byte{0x82},
                                            std::byte{0xa1},
                                            std::byte{'a'},
                                            std::byte{1},
                                            std::byte{0xa1},
                                            std::byte{'b'},
                                            std::byte{2}};
    const auto b = msgpack::Blob{std::span(buf)};
    auto &entries = std::get<msgpack::Map>(b.val);
    REQUIRE(entries.size() == 2);
    REQUIRE(std::get<std::string_view>(entries[0].first) == "a");
    REQUIRE(std::get<int64_t>(entries[0].second) == 1);
    REQUIRE(std::get<std::string_view>(entries[1].first) == "b");
    REQUIRE(std::get<int64_t>(entries[1].second) == 2);
  }
  // map16 with one entry { "x": 255 }
  {
    const auto buf = std::vector<std::byte>{std::byte{0xde},
                                            std::byte{0x00},
                                            std::byte{0x01},
                                            std::byte{0xa1},
                                            std::byte{'x'},
                                            std::byte{0xcc},
                                            std::byte{0xff}};
    const auto b = msgpack::Blob{std::span(buf)};
    const auto &e = std::get<msgpack::Map>(b.val);
    REQUIRE(e.size() == 1);
    REQUIRE(std::get<std::string_view>(e[0].first) == "x");
    REQUIRE(std::get<uint64_t>(e[0].second) == 255);
  }
}

TEST_CASE("Nested structures", "[msgpack]")
{
  // { "nums": [1,2], "m": { "k": true } }
  const auto buf = std::vector<std::byte>{std::byte{0x82},
                                          // "nums"
                                          std::byte{0xa4},
                                          std::byte{'n'},
                                          std::byte{'u'},
                                          std::byte{'m'},
                                          std::byte{'s'},
                                          // array of 2
                                          std::byte{0x92},
                                          std::byte{0x01},
                                          std::byte{0x02},
                                          // "m"
                                          std::byte{0xa1},
                                          std::byte{'m'},
                                          // map of 1
                                          std::byte{0x81},
                                          std::byte{0xa1},
                                          std::byte{'k'},
                                          std::byte{0xc3}};
  const auto b = msgpack::Blob{std::span(buf)};
  const auto &top = std::get<msgpack::Map>(b.val);
  // check "nums"
  const auto &nums = std::get<msgpack::Array>(top[0].second);
  REQUIRE(nums.size() == 2);
  REQUIRE(std::get<int64_t>(nums[0]) == 1);
  REQUIRE(std::get<int64_t>(nums[1]) == 2);
  // check "m"
  const auto &inner = std::get<msgpack::Map>(top[1].second);
  REQUIRE(inner.size() == 1);
  REQUIRE(std::get<std::string_view>(inner[0].first) == "k");
  REQUIRE(std::get<bool>(inner[0].second) == true);
}
