#include "volumeio.h"
#include <cstring>
#include <fstream>
#include <filesystem>

static constexpr char     kMagic[4]   = {'C','V','O','L'};
static constexpr uint32_t kVersion    = 1;
static constexpr uint32_t kDTypeFloat = 0;

bool saveVolume3D(const char* path, int N, int channels, const std::vector<float>& data) {
  const size_t expected = size_t(N) * N * N * channels; 
  if (data.size() != expected) return false;

  std::filesystem::path p(path);
  if (p.has_parent_path())
    std::filesystem::create_directories(p.parent_path());

  std::ofstream f(path, std::ios::binary);
  if (!f) return false;

  uint32_t w = (uint32_t) N, h = (uint32_t) N, d = (uint32_t) N;
  uint32_t c = (uint32_t) channels, dt = kDTypeFloat;

  f.write(kMagic, 4);
  f.write((const char*)&kVersion,4);
  f.write((const char*)&w, 4);
  f.write((const char*)&h, 4);
  f.write((const char*)&d, 4);
  f.write((const char*)&c, 4);
  f.write((const char*)&dt, 4);
  f.write((const char*)data.data(), data.size() * sizeof(float));
  return f.good();
}
bool loadVolume3D(const char* path, int& N, int& channels, std::vector<float>& data) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        std::fprintf(stderr, "loadVolume3D: cannot open '%s'\n", path);
        return false;
    }

    char magic[4];
    uint32_t version, w, h, d, c, dt;
    f.read(magic, 4);
    f.read((char*)&version, 4);
    f.read((char*)&w, 4);
    f.read((char*)&h, 4);
    f.read((char*)&d, 4);
    f.read((char*)&c, 4);
    f.read((char*)&dt, 4);
    if (!f) {
        std::fprintf(stderr, "loadVolume3D: short read in header of '%s'\n", path);
        return false;
    }

    if (std::memcmp(magic, kMagic, 4) != 0) {
        std::fprintf(stderr, "loadVolume3D: bad magic in '%s' (got %.4s)\n", path, magic);
        return false;
    }
    if (version != kVersion) {
        std::fprintf(stderr, "loadVolume3D: version %u, expected %u\n", version, kVersion);
        return false;
    }
    if (dt != kDTypeFloat) {
        std::fprintf(stderr, "loadVolume3D: dtype %u, expected %u\n", dt, kDTypeFloat);
        return false;
    }
    if (w != h || h != d) {
        std::fprintf(stderr, "loadVolume3D: non-cubic %ux%ux%u\n", w, h, d);
        return false;
    }

    N = (int)w;
    channels = (int)c;
    data.resize((size_t)w * h * d * c);
    f.read((char*)data.data(), data.size() * sizeof(float));
    if (!f) {
        std::fprintf(stderr, "loadVolume3D: payload short read (want %zu floats)\n",
                     data.size());
        return false;
    }
    return true;
}
