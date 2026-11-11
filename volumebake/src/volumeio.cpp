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

