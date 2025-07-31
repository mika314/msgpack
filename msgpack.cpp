#include "msgpack.hpp"
#include <cstring>
#include <stdexcept>

namespace msgpack
{
  namespace
  {
    template <typename UInt>
    UInt read_be(const std::span<const std::byte> &d, size_t off)
    {
      UInt v = 0;
      for (size_t i = 0; i < sizeof(UInt); ++i)
        v = static_cast<UInt>((v << 8) | static_cast<uint8_t>(d[off + i]));
      return v;
    }
  } // namespace

  Blob::Blob(std::istream &st)
    : blob([&st]() {
        st.unsetf(std::ios::skipws);
        std::vector<std::byte> r;
        char c;
        while (st.get(c))
          r.push_back(static_cast<std::byte>(static_cast<unsigned char>(c)));
        return r;
      }()),
      span(blob)
  {
    auto rem = parse(span, val);
    if (!rem.empty())
      throw std::runtime_error("Extra bytes after top‑level object");
  }

  Blob::Blob(std::span<const std::byte> s) : span(s)
  {
    auto rem = parse(span, val);
    if (!rem.empty())
      throw std::runtime_error("Extra bytes after top‑level object");
  }

  auto Blob::parse(std::span<const std::byte> in, Val &out) -> std::span<const std::byte>
  {
    if (in.empty())
      throw ParsingError("Unexpected EOF");

    const auto b = static_cast<uint8_t>(in[0]);

    // nil
    if (b == 0xc0)
    {
      out = nullptr;
      return in.subspan(1);
    }
    // bool
    if (b == 0xc2 || b == 0xc3)
    {
      out = (b == 0xc3);
      return in.subspan(1);
    }
    // positive fixint
    if (b <= 0x7f)
    {
      out = static_cast<int64_t>(b);
      return in.subspan(1);
    }
    // negative fixint
    if (b >= 0xe0)
    {
      out = static_cast<int64_t>(static_cast<int8_t>(b));
      return in.subspan(1);
    }
    // uint8
    if (b == 0xcc)
    {
      out = static_cast<uint64_t>(in[1]);
      return in.subspan(2);
    }
    // uint16
    if (b == 0xcd)
    {
      out = static_cast<uint64_t>(read_be<uint16_t>(in, 1));
      return in.subspan(3);
    }
    // uint32
    if (b == 0xce)
    {
      out = static_cast<uint64_t>(read_be<uint32_t>(in, 1));
      return in.subspan(5);
    }
    // uint64
    if (b == 0xcf)
    {
      out = static_cast<uint64_t>(read_be<uint64_t>(in, 1));
      return in.subspan(9);
    }
    // int8
    if (b == 0xd0)
    {
      out = static_cast<int64_t>(in[1]);
      return in.subspan(2);
    }
    // int16
    if (b == 0xd1)
    {
      out = static_cast<int64_t>(static_cast<int16_t>((read_be<uint16_t>(in, 1))));
      return in.subspan(3);
    }
    // int32
    if (b == 0xd2)
    {
      out = static_cast<int64_t>(static_cast<int32_t>(read_be<uint32_t>(in, 1)));
      return in.subspan(5);
    }
    // int64
    if (b == 0xd3)
    {
      out = static_cast<int64_t>(read_be<uint64_t>(in, 1));
      return in.subspan(9);
    }
    // float32
    if (b == 0xca)
    {
      uint32_t raw = read_be<uint32_t>(in, 1);
      float f;
      std::memcpy(&f, &raw, sizeof f);
      out = f;
      return in.subspan(5);
    }
    // float64
    if (b == 0xcb)
    {
      uint64_t raw = read_be<uint64_t>(in, 1);
      double d;
      std::memcpy(&d, &raw, sizeof d);
      out = d;
      return in.subspan(9);
    }
    // fixstr
    if ((b & 0xe0) == 0xa0)
    {
      uint32_t len = b & 0x1f;
      if (1 + len > in.size())
        throw ParsingError("String overflow");
      out = std::string_view(reinterpret_cast<const char *>(in.data() + 1), len);
      return in.subspan(1 + len);
    }
    // str8
    if (b == 0xd9)
    {
      auto len = read_be<uint8_t>(in, 1);
      auto start = in.subspan(2);
      if (len > start.size())
        throw ParsingError("str8 overflow");
      out = std::string_view(reinterpret_cast<const char *>(start.data()), len);
      return start.subspan(len);
    }
    // str16
    if (b == 0xda)
    {
      auto len = read_be<uint16_t>(in, 1);
      auto start = in.subspan(3);
      if (len > start.size())
        throw ParsingError("str16 overflow");
      out = std::string_view(reinterpret_cast<const char *>(start.data()), len);
      return start.subspan(len);
    }
    // str32
    if (b == 0xdb)
    {
      auto len = read_be<uint32_t>(in, 1);
      auto start = in.subspan(5);
      if (len > start.size())
        throw ParsingError("str32 overflow");
      out = std::string_view(reinterpret_cast<const char *>(start.data()), len);
      return start.subspan(len);
    }
    // bin8
    if (b == 0xc4)
    {
      auto len = read_be<uint8_t>(in, 1);
      auto start = in.subspan(2);
      if (len > start.size())
        throw ParsingError("bin8 overflow");
      out = std::span<const std::byte>(start.data(), len);
      return start.subspan(len);
    }
    // bin16
    if (b == 0xc5)
    {
      auto len = read_be<uint16_t>(in, 1);
      auto start = in.subspan(3);
      if (len > start.size())
        throw ParsingError("bin16 overflow");
      out = std::span<const std::byte>(start.data(), len);
      return start.subspan(len);
    }
    // bin32
    if (b == 0xc6)
    {
      auto len = read_be<uint32_t>(in, 1);
      auto start = in.subspan(5);
      if (len > start.size())
        throw ParsingError("bin32 overflow");
      out = std::span<const std::byte>(start.data(), len);
      return start.subspan(len);
    }
    // fixarray
    if ((b & 0xf0) == 0x90)
    {
      uint32_t n = b & 0x0f;
      Array a;
      auto cur = in.subspan(1);
      for (uint32_t i = 0; i < n; ++i)
      {
        Val e;
        cur = parse(cur, e);
        a.push_back(std::move(e));
      }
      out = std::move(a);
      return cur;
    }
    // array16
    if (b == 0xdc)
    {
      auto n = read_be<uint16_t>(in, 1);
      auto cur = in.subspan(3);
      Array a;
      for (uint16_t i = 0; i < n; ++i)
      {
        Val e;
        cur = parse(cur, e);
        a.push_back(std::move(e));
      }
      out = std::move(a);
      return cur;
    }
    // array32
    if (b == 0xdd)
    {
      auto n = read_be<uint32_t>(in, 1);
      auto cur = in.subspan(5);
      Array a;
      for (uint32_t i = 0; i < n; ++i)
      {
        Val e;
        cur = parse(cur, e);
        a.push_back(std::move(e));
      }
      out = std::move(a);
      return cur;
    }
    // fixmap
    if ((b & 0xf0) == 0x80)
    {
      uint32_t n = b & 0x0f;
      Map m;
      auto cur = in.subspan(1);
      for (uint32_t i = 0; i < n; ++i)
      {
        Val k, v;
        cur = parse(cur, k);
        cur = parse(cur, v);
        m.emplace_back(std::move(k), std::move(v));
      }
      out = std::move(m);
      return cur;
    }
    // map16
    if (b == 0xde)
    {
      auto n = read_be<uint16_t>(in, 1);
      auto cur = in.subspan(3);
      Map m;
      for (uint16_t i = 0; i < n; ++i)
      {
        Val k, v;
        cur = parse(cur, k);
        cur = parse(cur, v);
        m.emplace_back(std::move(k), std::move(v));
      }
      out = std::move(m);
      return cur;
    }
    // map32
    if (b == 0xdf)
    {
      auto n = read_be<uint32_t>(in, 1);
      auto cur = in.subspan(5);
      Map m;
      for (uint32_t i = 0; i < n; ++i)
      {
        Val k, v;
        cur = parse(cur, k);
        cur = parse(cur, v);
        m.emplace_back(std::move(k), std::move(v));
      }
      out = std::move(m);
      return cur;
    }

    throw ParsingError("Unknown type byte " + std::to_string(b));
  }

  ParsingError::~ParsingError() = default;
} // namespace msgpack
