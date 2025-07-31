// (c) 2025 Mika Pi

#pragma once
#include <cstring>
#include <iostream>
#include <map>
#include <ser/is_serializable.hpp>
#include <sstream>
#include <unordered_map>

#include "msgpack.hpp"

template <typename T>
auto msgpackSer(std::ostream &st, T v) -> void;

template <typename T>
auto msgpackDeser(const msgpack::Val &jv, T &v) -> void;

namespace Internal
{
  template <typename T>
  struct IsVariant : std::false_type
  {
  };

  template <typename... Args>
  struct IsVariant<std::variant<Args...>> : std::true_type
  {
  };

  auto msgpackSerVal(std::ostream &st, std::string v) -> void;

  auto get_type_name(const msgpack::Val &v) -> std::string;

  template <typename T>
  auto msgpackSerVal(std::ostream &st, T v)
    -> std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_enum_v<T>>
  {
    if constexpr (std::is_floating_point_v<T>)
    {
      if constexpr (sizeof(T) == 4)
      {
        st.put(static_cast<char>(0xca));
        uint32_t raw;
        std::memcpy(&raw, &v, sizeof(v));
        st.put(static_cast<char>(raw >> 24));
        st.put(static_cast<char>(raw >> 16));
        st.put(static_cast<char>(raw >> 8));
        st.put(static_cast<char>(raw));
      }
      else
      {
        st.put(static_cast<char>(0xcb));
        uint64_t raw;
        std::memcpy(&raw, &v, sizeof(v));
        st.put(static_cast<char>(raw >> 56));
        st.put(static_cast<char>(raw >> 48));
        st.put(static_cast<char>(raw >> 40));
        st.put(static_cast<char>(raw >> 32));
        st.put(static_cast<char>(raw >> 24));
        st.put(static_cast<char>(raw >> 16));
        st.put(static_cast<char>(raw >> 8));
        st.put(static_cast<char>(raw));
      }
    }
    else if constexpr (std::is_signed_v<T>)
    {
      int64_t i = v;
      if (i >= 0)
      {
        if (i < 128)
        {
          st.put(static_cast<char>(i));
        }
        else if (i < 256)
        {
          st.put(static_cast<char>(0xcc));
          st.put(static_cast<char>(i));
        }
        else if (i < 65536)
        {
          st.put(static_cast<char>(0xcd));
          st.put(static_cast<char>(i >> 8));
          st.put(static_cast<char>(i));
        }
        else if (i < 4294967296)
        {
          st.put(static_cast<char>(0xce));
          st.put(static_cast<char>(i >> 24));
          st.put(static_cast<char>(i >> 16));
          st.put(static_cast<char>(i >> 8));
          st.put(static_cast<char>(i));
        }
        else
        {
          st.put(static_cast<char>(0xcf));
          st.put(static_cast<char>(i >> 56));
          st.put(static_cast<char>(i >> 48));
          st.put(static_cast<char>(i >> 40));
          st.put(static_cast<char>(i >> 32));
          st.put(static_cast<char>(i >> 24));
          st.put(static_cast<char>(i >> 16));
          st.put(static_cast<char>(i >> 8));
          st.put(static_cast<char>(i));
        }
      }
      else
      {
        if (i >= -32)
        {
          st.put(static_cast<char>(i));
        }
        else if (i >= -128)
        {
          st.put(static_cast<char>(0xd0));
          st.put(static_cast<char>(i));
        }
        else if (i >= -32768)
        {
          st.put(static_cast<char>(0xd1));
          st.put(static_cast<char>(i >> 8));
          st.put(static_cast<char>(i));
        }
        else if (i >= -2147483648)
        {
          st.put(static_cast<char>(0xd2));
          st.put(static_cast<char>(i >> 24));
          st.put(static_cast<char>(i >> 16));
          st.put(static_cast<char>(i >> 8));
          st.put(static_cast<char>(i));
        }
        else
        {
          st.put(static_cast<char>(0xd3));
          st.put(static_cast<char>(i >> 56));
          st.put(static_cast<char>(i >> 48));
          st.put(static_cast<char>(i >> 40));
          st.put(static_cast<char>(i >> 32));
          st.put(static_cast<char>(i >> 24));
          st.put(static_cast<char>(i >> 16));
          st.put(static_cast<char>(i >> 8));
          st.put(static_cast<char>(i));
        }
      }
    }
    else
    {
      uint64_t i = v;
      if (i < 128)
      {
        st.put(static_cast<char>(i));
      }
      else if (i < 256)
      {
        st.put(static_cast<char>(0xcc));
        st.put(static_cast<char>(i));
      }
      else if (i < 65536)
      {
        st.put(static_cast<char>(0xcd));
        st.put(static_cast<char>(i >> 8));
        st.put(static_cast<char>(i));
      }
      else if (i < 4294967296)
      {
        st.put(static_cast<char>(0xce));
        st.put(static_cast<char>(i >> 24));
        st.put(static_cast<char>(i >> 16));
        st.put(static_cast<char>(i >> 8));
        st.put(static_cast<char>(i));
      }
      else
      {
        st.put(static_cast<char>(0xcf));
        st.put(static_cast<char>(i >> 56));
        st.put(static_cast<char>(i >> 48));
        st.put(static_cast<char>(i >> 40));
        st.put(static_cast<char>(i >> 32));
        st.put(static_cast<char>(i >> 24));
        st.put(static_cast<char>(i >> 16));
        st.put(static_cast<char>(i >> 8));
        st.put(static_cast<char>(i));
      }
    }
  }

  template <typename T>
  auto msgpackSerVal(std::ostream &st, std::vector<T> v) -> void
  {
    if (v.size() < 16)
    {
      st.put(static_cast<char>(0x90 | v.size()));
    }
    else if (v.size() < 65536)
    {
      st.put(static_cast<char>(0xdc));
      st.put(static_cast<char>(v.size() >> 8));
      st.put(static_cast<char>(v.size()));
    }
    else
    {
      st.put(static_cast<char>(0xdd));
      st.put(static_cast<char>(v.size() >> 24));
      st.put(static_cast<char>(v.size() >> 16));
      st.put(static_cast<char>(v.size() >> 8));
      st.put(static_cast<char>(v.size()));
    }
    for (auto &&e : v)
    {
      msgpackSer(st, std::move(e));
    }
  }

  auto msgpackSerVal(std::ostream &st, bool v) -> void;

  template <typename... Ts>
  auto msgpackSerVal(std::ostream &st, std::variant<Ts...> v) -> void
  {
    std::visit([&](auto vv) { msgpackSer(st, std::move(vv)); }, v);
  }

  template <typename T>
  auto msgpackSerVal(std::ostream &st, std::unordered_map<std::string, T> v) -> void
  {
    if (v.size() < 16)
    {
      st.put(static_cast<char>(0x80 | v.size()));
    }
    else if (v.size() < 65536)
    {
      st.put(static_cast<char>(0xde));
      st.put(static_cast<char>(v.size() >> 8));
      st.put(static_cast<char>(v.size()));
    }
    else
    {
      st.put(static_cast<char>(0xdf));
      st.put(static_cast<char>(v.size() >> 24));
      st.put(static_cast<char>(v.size() >> 16));
      st.put(static_cast<char>(v.size() >> 8));
      st.put(static_cast<char>(v.size()));
    }
    for (auto &&e : v)
    {
      Internal::msgpackSerVal(st, e.first);
      msgpackSer(st, std::move(e.second));
    }
  }

  template <typename T>
  auto msgpackSerVal(std::ostream &st, std::map<std::string, T> v) -> void
  {
    if (v.size() < 16)
    {
      st.put(static_cast<char>(0x80 | v.size()));
    }
    else if (v.size() < 65536)
    {
      st.put(static_cast<char>(0xde));
      st.put(static_cast<char>(v.size() >> 8));
      st.put(static_cast<char>(v.size()));
    }
    else
    {
      st.put(static_cast<char>(0xdf));
      st.put(static_cast<char>(v.size() >> 24));
      st.put(static_cast<char>(v.size() >> 16));
      st.put(static_cast<char>(v.size() >> 8));
      st.put(static_cast<char>(v.size()));
    }
    for (auto &&e : v)
    {
      Internal::msgpackSerVal(st, e.first);
      msgpackSer(st, std::move(e.second));
    }
  }

  template <typename U, typename T>
  auto msgpackSerVal(std::ostream &st, std::unordered_map<U, T> v)
    -> std::enable_if_t<std::is_integral_v<U> || std::is_enum_v<U>>
  {
    if (v.size() < 16)
    {
      st.put(static_cast<char>(0x80 | v.size()));
    }
    else if (v.size() < 65536)
    {
      st.put(static_cast<char>(0xde));
      st.put(static_cast<char>(v.size() >> 8));
      st.put(static_cast<char>(v.size()));
    }
    else
    {
      st.put(static_cast<char>(0xdf));
      st.put(static_cast<char>(v.size() >> 24));
      st.put(static_cast<char>(v.size() >> 16));
      st.put(static_cast<char>(v.size() >> 8));
      st.put(static_cast<char>(v.size()));
    }
    for (auto &&e : v)
    {
      Internal::msgpackSerVal(st, e.first);
      msgpackSer(st, std::move(e.second));
    }
  }

  template <typename U, typename T>
  auto msgpackSerVal(std::ostream &st, std::map<U, T> v)
    -> std::enable_if_t<std::is_integral_v<U> || std::is_enum_v<U>>
  {
    if (v.size() < 16)
    {
      st.put(static_cast<char>(0x80 | v.size()));
    }
    else if (v.size() < 65536)
    {
      st.put(static_cast<char>(0xde));
      st.put(static_cast<char>(v.size() >> 8));
      st.put(static_cast<char>(v.size()));
    }
    else
    {
      st.put(static_cast<char>(0xdf));
      st.put(static_cast<char>(v.size() >> 24));
      st.put(static_cast<char>(v.size() >> 16));
      st.put(static_cast<char>(v.size() >> 8));
      st.put(static_cast<char>(v.size()));
    }
    for (auto &&e : v)
    {
      Internal::msgpackSerVal(st, e.first);
      msgpackSer(st, std::move(e.second));
    }
  }

  auto msgpackDeserVal(const msgpack::Val &j, std::string &v) -> void;

  template <typename T>
  auto msgpackDeserVal(const msgpack::Val &j, T &v)
    -> std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_enum_v<T>>
  {
    if constexpr (std::is_floating_point_v<T>)
    {
      if constexpr (sizeof(T) == 4)
      {
        if (!std::holds_alternative<float>(j))
          throw msgpack::ParsingError{"Type mismatch. Expected float, got " + get_type_name(j)};
        v = std::get<float>(j);
      }
      else
      {
        if (!std::holds_alternative<double>(j))
          throw msgpack::ParsingError{"Type mismatch. Expected double, got " + get_type_name(j)};
        v = std::get<double>(j);
      }
    }
    else
    {
      if (std::holds_alternative<uint64_t>(j))
        v = static_cast<T>(std::get<uint64_t>(j));
      else if (std::holds_alternative<int64_t>(j))
        v = static_cast<T>(std::get<int64_t>(j));
      else
        throw msgpack::ParsingError{"Type mismatch. Expected integer, got " + get_type_name(j)};
    }
  }

  template <typename T>
  auto msgpackDeserVal(const msgpack::Val &j, std::vector<T> &v) -> void
  {
    if (!std::holds_alternative<msgpack::Array>(j))
      throw msgpack::ParsingError{"Type mismatch. Expected Array, got " + get_type_name(j)};
    const auto &arr = std::get<msgpack::Array>(j);
    v.clear();
    for (const auto &e : arr)
      msgpackDeser(e, v.emplace_back());
  }

  auto msgpackDeserVal(const msgpack::Val &j, bool &v) -> void;

  template <auto N = 0, typename... Ts>
  auto msgpackDeserVal(const msgpack::Val &j, size_t idx, std::variant<Ts...> &v) -> void
  {
    if constexpr (N >= sizeof...(Ts))
      return;
    else
    {
      if (idx == N)
        msgpackDeser(j, v.template emplace<N>());
      else
        msgpackDeserVal<N + 1, Ts...>(j, idx, v);
    }
  }

  template <typename T>
  auto msgpackDeserVal(const msgpack::Val &j, std::unordered_map<std::string, T> &v) -> void
  {
    if (!std::holds_alternative<msgpack::Map>(j))
      throw msgpack::ParsingError{"Type mismatch. Expected Map, got " + get_type_name(j)};
    const auto &map = std::get<msgpack::Map>(j);
    for (const auto &e : map)
    {
      if (!std::holds_alternative<std::string_view>(e.first))
        throw msgpack::ParsingError{"Type mismatch. Expected string_view, got " +
                                    get_type_name(e.first)};
      auto key = std::string{std::get<std::string_view>(e.first)};
      auto tmp = v.emplace(key, T{});
      msgpackDeser(e.second, tmp.first->second);
    }
  }

  template <typename T>
  auto msgpackDeserVal(const msgpack::Val &j, std::map<std::string, T> &v) -> void
  {
    if (!std::holds_alternative<msgpack::Map>(j))
      throw msgpack::ParsingError{"Type mismatch. Expected Map, got " + get_type_name(j)};
    const auto &map = std::get<msgpack::Map>(j);
    for (const auto &e : map)
    {
      if (!std::holds_alternative<std::string_view>(e.first))
        throw msgpack::ParsingError{"Type mismatch. Expected string_view, got " +
                                    get_type_name(e.first)};
      auto key = std::string{std::get<std::string_view>(e.first)};
      auto tmp = v.emplace(key, T{});
      msgpackDeser(e.second, tmp.first->second);
    }
  }

  template <typename U, typename T>
  auto msgpackDeserVal(const msgpack::Val &j, std::unordered_map<U, T> &v)
    -> std::enable_if_t<std::is_integral_v<U> || std::is_enum_v<U>>
  {
    if (!std::holds_alternative<msgpack::Map>(j))
      throw msgpack::ParsingError{"Type mismatch. Expected Map, got " + get_type_name(j)};
    const auto &map = std::get<msgpack::Map>(j);
    for (const auto &e : map)
    {
      U key;
      msgpackDeserVal(e.first, key);
      auto tmp = v.emplace(key, T{});
      msgpackDeser(e.second, tmp.first->second);
    }
  }

  template <typename U, typename T>
  auto msgpackDeserVal(const msgpack::Val &j, std::map<U, T> &v)
    -> std::enable_if_t<std::is_integral_v<U> || std::is_enum_v<U>>
  {
    if (!std::holds_alternative<msgpack::Map>(j))
      throw msgpack::ParsingError{"Type mismatch. Expected Map, got " + get_type_name(j)};
    const auto &map = std::get<msgpack::Map>(j);
    for (const auto &e : map)
    {
      U key;
      msgpackDeserVal(e.first, key);
      auto tmp = v.emplace(key, T{});
      msgpackDeser(e.second, tmp.first->second);
    }
  }

  // null
  // optional

  struct MsgpackArch
  {
    MsgpackArch(const msgpack::Map &aMap) : map(aMap), index(0) {}

    template <typename T>
    auto operator()([[maybe_unused]] const char *name, T &vv) -> void
    {
      if (index >= map.size())
        return;

      if constexpr (Internal::IsVariant<T>::value)
      {
        size_t type_idx = 0;
        const auto &type_val = map[index++].second;
        if (std::holds_alternative<uint64_t>(type_val))
          type_idx = static_cast<size_t>(std::get<uint64_t>(type_val));
        else if (std::holds_alternative<int64_t>(type_val))
          type_idx = static_cast<size_t>(std::get<int64_t>(type_val));

        const auto &val_to_deser = map[index].second;
        Internal::msgpackDeserVal(val_to_deser, type_idx, vv);
        index++;
      }
      else
      {
        const auto &val_to_deser = map[index].second;
        if constexpr (IsSerializableClassV<T>)
        {
          msgpackDeser(val_to_deser, vv);
        }
        else
        {
          msgpackDeserVal(val_to_deser, vv);
        }
        index++;
      }
    }
    const msgpack::Map &map;
    size_t index;
  };
} // namespace Internal

template <typename T>
auto msgpackSer(std::ostream &st, T v) -> void
{
  if constexpr (IsSerializableClassV<T>)
  {
    std::stringstream sst;
    size_t count = 0;
    auto l = [&](const char *name, auto vv) mutable {
      if constexpr (Internal::IsVariant<decltype(vv)>::value)
      {
        Internal::msgpackSerVal(sst, std::string(name) + "Type");
        msgpackSer(sst, vv.index());
        count++;
      }
      Internal::msgpackSerVal(sst, name);
      msgpackSer(sst, std::move(vv));
      count++;
    };
    v.ser(l);

    if (count < 16)
    {
      st.put(static_cast<char>(0x80 | count));
    }
    else if (count < 65536)
    {
      st.put(static_cast<char>(0xde));
      st.put(static_cast<char>(count >> 8));
      st.put(static_cast<char>(count));
    }
    else
    {
      st.put(static_cast<char>(0xdf));
      st.put(static_cast<char>(count >> 24));
      st.put(static_cast<char>(count >> 16));
      st.put(static_cast<char>(count >> 8));
      st.put(static_cast<char>(count));
    }
    st << sst.rdbuf();
  }
  else
    Internal::msgpackSerVal(st, std::move(v));
}

template <typename T>
auto msgpackDeser(const msgpack::Val &jv, T &v) -> void
{
  if constexpr (IsSerializableClassV<T>)
  {
    auto arch = Internal::MsgpackArch{std::get<msgpack::Map>(jv)};
    v.deser(arch);
  }
  else
    Internal::msgpackDeserVal(jv, v);
}

template <typename T>
auto msgpackDeser(std::istream &st, T &v) -> void
{
  static_assert(IsSerializableClassV<T>);
  auto root = msgpack::Blob{st};
  msgpackDeser(root.val, v);
}
