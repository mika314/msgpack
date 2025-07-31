// (c) 2025 Mika Pi

#include "msgpack-ser.hpp"

namespace Internal
{
  auto msgpackSerVal(std::ostream &st, std::string v) -> void
  {
    if (v.size() < 32)
    {
      st.put(static_cast<char>(0xa0 | v.size()));
    }
    else if (v.size() < 256)
    {
      st.put(static_cast<char>(0xd9));
      st.put(static_cast<char>(v.size()));
    }
    else if (v.size() < 65536)
    {
      st.put(static_cast<char>(0xda));
      st.put(static_cast<char>(v.size() >> 8));
      st.put(static_cast<char>(v.size()));
    }
    else
    {
      st.put(static_cast<char>(0xdb));
      st.put(static_cast<char>(v.size() >> 24));
      st.put(static_cast<char>(v.size() >> 16));
      st.put(static_cast<char>(v.size() >> 8));
      st.put(static_cast<char>(v.size()));
    }
    st.write(v.data(), static_cast<std::streamsize>(v.size()));
  }

  auto msgpackSerVal(std::ostream &st, bool v) -> void
  {
    st.put(static_cast<char>(v ? 0xc3 : 0xc2));
  }

  auto msgpackDeserVal(const msgpack::Val &j, std::string &v) -> void
  {
    if (!std::holds_alternative<std::string_view>(j))
      return;
    v = std::get<std::string_view>(j);
  }

  auto msgpackDeserVal(const msgpack::Val &j, bool &v) -> void
  {
    if (!std::holds_alternative<bool>(j))
      return;
    v = std::get<bool>(j);
  }

  auto get_type_name(const msgpack::Val &v) -> std::string
  {
    return std::visit(
      [](auto &&arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, int64_t>)
          return "int64_t";
        else if constexpr (std::is_same_v<T, uint64_t>)
          return "uint64_t";
        else if constexpr (std::is_same_v<T, std::nullptr_t>)
          return "nullptr_t";
        else if constexpr (std::is_same_v<T, bool>)
          return "bool";
        else if constexpr (std::is_same_v<T, float>)
          return "float";
        else if constexpr (std::is_same_v<T, double>)
          return "double";
        else if constexpr (std::is_same_v<T, std::string_view>)
          return "string_view";
        else if constexpr (std::is_same_v<T, std::span<const std::byte>>)
          return "span<const std::byte>";
        else if constexpr (std::is_same_v<T, msgpack::Array>)
          return "Array";
        else if constexpr (std::is_same_v<T, msgpack::Map>)
          return "Map";
      },
      v);
  }
} // namespace Internal
