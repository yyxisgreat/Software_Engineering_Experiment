#include "package_export.h"
#include "binary_io.h"
#include "compress_rle.h"
#include "encrypt_xor.h"
#include "encrypt_rc4.h"
#include "pack_header.h"
#include "pack_toc.h"

#include <fstream>
#include <random>
#include <vector>
#include <stdexcept>

namespace pkg {

static std::vector<uint8_t> gen_salt(size_t n = 16) {
    std::vector<uint8_t> s(n);
    std::random_device rd;
    for (size_t i = 0; i < n; ++i) s[i] = static_cast<uint8_t>(rd() & 0xFF);
    return s;
}

static std::vector<uint8_t> read_file_all(const std::filesystem::path& p) {
    std::ifstream ifs(p, std::ios::binary);
    if (!ifs) throw std::runtime_error("open file failed: " + p.string());
    ifs.seekg(0, std::ios::end);
    auto n = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    std::vector<uint8_t> buf(static_cast<size_t>(n));
    if (n > 0) ifs.read(reinterpret_cast<char*>(buf.data()), n);
    if (!ifs) throw std::runtime_error("read file failed: " + p.string());
    return buf;
}

static void write_file_all(const std::filesystem::path& p, const std::vector<uint8_t>& buf) {
    std::filesystem::create_directories(p.parent_path());
    std::ofstream ofs(p, std::ios::binary);
    if (!ofs) throw std::runtime_error("write file failed: " + p.string());
    if (!buf.empty()) ofs.write(reinterpret_cast<const char*>(buf.data()),
                                static_cast<std::streamsize>(buf.size()));
}

static std::string to_rel_generic(const std::filesystem::path& base,
                                  const std::filesystem::path& p) {
    auto rel = std::filesystem::relative(p, base);
    return rel.generic_string(); // 强制用 /
}

static std::vector<uint8_t> apply_compress(const std::vector<uint8_t>& in, CompressAlg alg) {
    if (alg == CompressAlg::RLE) return rle_compress(in);
    return in;
}
static std::vector<uint8_t> apply_decompress(const std::vector<uint8_t>& in, CompressAlg alg) {
    if (alg == CompressAlg::RLE) return rle_decompress(in);
    return in;
}
static std::vector<uint8_t> apply_encrypt(const std::vector<uint8_t>& in, EncryptAlg alg,
                                          const std::string& pw, const std::vector<uint8_t>& salt) {
    if (alg == EncryptAlg::XOR) return xor_crypt(in, pw, salt);
    if (alg == EncryptAlg::RC4) return rc4_crypt(in, pw, salt);
    return in;
}
static std::vector<uint8_t> apply_decrypt(const std::vector<uint8_t>& in, EncryptAlg alg,
                                          const std::string& pw, const std::vector<uint8_t>& salt) {
    // XOR/RC4 都是对称加密：同一个函数
    return apply_encrypt(in, alg, pw, salt);
}

// 文件头： "SEXP01"(6) + ver(u8) + pack(u8) + comp(u8) + enc(u8) + saltLen(u32) + saltBytes
static const char MAGIC[6] = {'S','E','X','P','0','1'};

bool export_repo_to_package(const std::filesystem::path& repoDir,
                            const std::filesystem::path& packageFile,
                            const Options& opt) {
    if (!std::filesystem::exists(repoDir))
        throw std::runtime_error("repoDir not exist: " + repoDir.string());

    if (opt.encryptAlg != EncryptAlg::None && opt.password.empty())
        throw std::runtime_error("encrypt enabled but password is empty");

    auto salt = (opt.encryptAlg == EncryptAlg::None) ? std::vector<uint8_t>{} : gen_salt(16);

    // 收集 repoDir 下所有普通文件（包含 index.txt / data/...）
    std::vector<Entry> entries;
    for (auto& it : std::filesystem::recursive_directory_iterator(repoDir)) {
        if (!it.is_regular_file()) continue;

        auto abs = it.path();

        // 避免把输出包自己又打进去（如果你把包输出到 repoDir 里）
        // ⚠️ 注意：std::filesystem::equivalent 要求两个路径都存在，否则会抛异常
        try {
            if (std::filesystem::exists(packageFile) && std::filesystem::equivalent(abs, packageFile)) {
                continue;
            }
        } catch (...) {
            // 忽略等价性检查错误（比如 packageFile 尚未创建时）
        }

        auto raw = read_file_all(abs);

        Entry e;
        e.relPath = to_rel_generic(repoDir, abs);
        e.originalSize = static_cast<uint64_t>(raw.size());

        auto c = apply_compress(raw, opt.compressAlg);
        auto enc = apply_encrypt(c, opt.encryptAlg, opt.password, salt);

        e.payload = std::move(enc);
        entries.push_back(std::move(e));
    }

    std::ofstream os(packageFile, std::ios::binary);
    if (!os) throw std::runtime_error("cannot create package file: " + packageFile.string());

    // 写头
    os.write(MAGIC, 6);
    write_u8(os, 1); // version
    write_u8(os, static_cast<uint8_t>(opt.packAlg));
    write_u8(os, static_cast<uint8_t>(opt.compressAlg));
    write_u8(os, static_cast<uint8_t>(opt.encryptAlg));
    write_le<uint32_t>(os, static_cast<uint32_t>(salt.size()));
    write_bytes(os, salt);

    // 写包体
    if (opt.packAlg == PackAlg::HeaderPerFile) {
        pack_header_write(os, entries);
    } else {
        std::vector<TocItem> toc;
        std::vector<std::vector<uint8_t>> blobs;
        toc.reserve(entries.size());
        blobs.reserve(entries.size());

        for (auto& e : entries) {
            TocItem t;
            t.relPath = e.relPath;
            t.originalSize = e.originalSize;

            toc.push_back(std::move(t));
            blobs.push_back(std::move(e.payload));
        }

        pack_toc_write(os, toc, blobs);
    }

    return true;
}

bool import_package_to_repo(const std::filesystem::path& packageFile,
                            const std::filesystem::path& repoDir,
                            const std::string& password) {
    std::ifstream is(packageFile, std::ios::binary);
    if (!is) throw std::runtime_error("cannot open package file: " + packageFile.string());

    // 读 magic
    char magic[6];
    is.read(magic, 6);
    if (!is || std::string(magic, 6) != std::string(MAGIC, 6))
        throw std::runtime_error("magic mismatch");

    auto ver = read_u8(is);
    (void)ver;

    PackAlg packAlg = static_cast<PackAlg>(read_u8(is));
    CompressAlg compAlg = static_cast<CompressAlg>(read_u8(is));
    EncryptAlg encAlg = static_cast<EncryptAlg>(read_u8(is));

    uint32_t saltLen = read_le<uint32_t>(is);
    auto salt = read_bytes(is, saltLen);

    if (encAlg != EncryptAlg::None && password.empty())
        throw std::runtime_error("package is encrypted but password is empty");

    std::filesystem::create_directories(repoDir);

    if (packAlg == PackAlg::HeaderPerFile) {
        auto entries = pack_header_read(is);
        for (auto& e : entries) {
            auto dec = apply_decrypt(e.payload, encAlg, password, salt);
            auto raw = apply_decompress(dec, compAlg);

            auto outPath = repoDir / std::filesystem::path(e.relPath);
            write_file_all(outPath, raw);
        }
    } else {
        std::vector<TocItem> toc;
        std::vector<std::vector<uint8_t>> blobs;

        pack_toc_read(is, toc, blobs);

        for (size_t i = 0; i < toc.size(); ++i) {
            auto dec = apply_decrypt(blobs[i], encAlg, password, salt);
            auto raw = apply_decompress(dec, compAlg);

            auto outPath = repoDir / std::filesystem::path(toc[i].relPath);
            write_file_all(outPath, raw);
        }
    }

    return true;
}

} // namespace pkg
