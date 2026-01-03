#pragma once
#include <cstdint>
#include <istream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace pkg {

inline void write_u8(std::ostream& os, uint8_t v) { os.put(static_cast<char>(v)); }

inline uint8_t read_u8(std::istream& is) {
    int c = is.get();
    if (c == EOF) throw std::runtime_error("read_u8: EOF");
    return static_cast<uint8_t>(c);
}

template <typename T>
inline void write_le(std::ostream& os, T v) {
    static_assert(std::is_integral<T>::value, "write_le requires integral");
    for (size_t i = 0; i < sizeof(T); ++i) {
        os.put(static_cast<char>((static_cast<uint64_t>(v) >> (8 * i)) & 0xFF));
    }
}

template <typename T>
inline T read_le(std::istream& is) {
    static_assert(std::is_integral<T>::value, "read_le requires integral");
    uint64_t v = 0;
    for (size_t i = 0; i < sizeof(T); ++i) {
        int c = is.get();
        if (c == EOF) throw std::runtime_error("read_le: EOF");
        v |= (static_cast<uint64_t>(static_cast<uint8_t>(c)) << (8 * i));
    }
    return static_cast<T>(v);
}

inline void write_bytes(std::ostream& os, const std::vector<uint8_t>& buf) {
    if (!buf.empty()) os.write(reinterpret_cast<const char*>(buf.data()),
                               static_cast<std::streamsize>(buf.size()));
}

inline std::vector<uint8_t> read_bytes(std::istream& is, size_t n) {
    std::vector<uint8_t> buf(n);
    if (n > 0) is.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(n));
    if (!is) throw std::runtime_error("read_bytes: failed");
    return buf;
}

inline void write_string(std::ostream& os, const std::string& s) {
    write_le<uint32_t>(os, static_cast<uint32_t>(s.size()));
    if (!s.empty()) os.write(s.data(), static_cast<std::streamsize>(s.size()));
}

inline std::string read_string(std::istream& is) {
    uint32_t n = read_le<uint32_t>(is);
    std::string s(n, '\0');
    if (n > 0) is.read(&s[0], static_cast<std::streamsize>(n));
    if (!is) throw std::runtime_error("read_string: failed");
    return s;
}

} // namespace pkg
