#pragma once
#include <cstddef>
#include <iostream>
#include <span>
#include <string_view>
#include <variant>
#include <vector>

namespace msgpack
{
  class Map;
  class Array;
  using Val = std::variant<int64_t,
                           uint64_t,
                           std::nullptr_t,
                           bool,
                           float,
                           double,
                           std::string_view,
                           std::span<const std::byte>,
                           Array,
                           Map>;
  class Array
  {
  public:
    std::vector<Val> items;
  };

  class Map
  {
  public:
    std::vector<std::pair<Val, Val>> entries;
  };

  class Blob
  {
  private:
    std::vector<std::byte> blob;
    std::span<const std::byte> span;
    auto parse(std::span<const std::byte> in, Val &out) -> std::span<const std::byte>;

  public:
    Blob(std::istream &);
    Blob(std::span<const std::byte>);
    Val val;
  };
} // namespace msgpack
