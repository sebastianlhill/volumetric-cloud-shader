#include <glm/glm.hpp>
#include "TileableVolumeNoise.h"
#include "cloudbake.h"
#include <vector>
#include <algorithm>


// Cloud shape: RGBA float, row-major, texel = 4 floats
//   R = Perlin-Worley
//   G = Worley FBM (low)
//   B = Worley FBM (mid)
//   A = Worley FBM (high)
std::vector<float> bakeCloudShape(int N) {
    std::vector<float> data(size_t(N) * N * N * 4);
    const float freqMul[6] = { 2.0f, 8.0f, 14.0f, 20.0f, 26.0f, 32.0f };
    const float normFact  = 1.0f / float(N);

    for (int z = 0; z < N; ++z) {
        for (int y = 0; y < N; ++y) {
            for (int x = 0; x < N; ++x) {
                glm::vec3 coord = glm::vec3(x, y, z) * normFact; // [0,1]

                // ---- Perlin FBM (3 octaves, freq 8) ----
                float perlinNoise = Tileable3dNoise::PerlinNoise(coord, 8.0f, 3);

                // ---- Worley FBM used *inside* the Perlin-Worley remap ----
                // (Hillaire computes 6, uses only the first 3 — the rest are dead
                //  code in the reference. We skip them; the result is identical.)
                const float cellCount = 4.0f;
                float w0 = 1.0f - Tileable3dNoise::WorleyNoise(coord, cellCount * freqMul[0]);
                float w1 = 1.0f - Tileable3dNoise::WorleyNoise(coord, cellCount * freqMul[1]);
                float w2 = 1.0f - Tileable3dNoise::WorleyNoise(coord, cellCount * freqMul[2]);
                float worleyFBM = w0 * 0.625f + w1 * 0.25f + w2 * 0.125f;

                // Perlin-Worley (GPU Pro 7 p.101): remap(perlin, 0, 1, worleyFBM, 1)
                float perlinWorley = remap(perlinNoise, 0.0f, 1.0f, worleyFBM, 1.0f);

                // ---- Second Worley batch: the G/B/A channels ----
                float wa = 1.0f - Tileable3dNoise::WorleyNoise(coord, cellCount * 1.0f);
                float wb = 1.0f - Tileable3dNoise::WorleyNoise(coord, cellCount * 2.0f);
                float wc = 1.0f - Tileable3dNoise::WorleyNoise(coord, cellCount * 4.0f);
                float wd = 1.0f - Tileable3dNoise::WorleyNoise(coord, cellCount * 8.0f);
                float we = 1.0f - Tileable3dNoise::WorleyNoise(coord, cellCount * 16.0f);

                float fbm0 = wb * 0.625f + wc * 0.25f + wd * 0.125f; // freq 8/16/32
                float fbm1 = wc * 0.625f + wd * 0.25f + we * 0.125f; // freq 16/32/64
                float fbm2 = wd * 0.75f  + we * 0.25f;               // freq 32/64

                size_t a = (size_t(z) * N + y) * N + x;
                data[a*4 + 0] = std::clamp(perlinWorley, 0.0f, 1.0f);
                data[a*4 + 1] = std::clamp(fbm0,        0.0f, 1.0f);
                data[a*4 + 2] = std::clamp(fbm1,        0.0f, 1.0f);
                data[a*4 + 3] = std::clamp(fbm2,        0.0f, 1.0f);
            }
        }
    }
    return data;
}

// Erosion: RGBA float, texel = 4 floats
//   R/G/B = Worley FBM at 3 higher frequencies, A = 1.0
std::vector<float> bakeCloudErosion(int N) {
    std::vector<float> data(size_t(N) * N * N * 4);
    const float normFact = 1.0f / float(N);
    const float cellCount = 2.0f;

    for (int z = 0; z < N; ++z) {
        for (int y = 0; y < N; ++y) {
            for (int x = 0; x < N; ++x) {
                glm::vec3 coord = glm::vec3(x, y, z) * normFact;

                float w0 = 1.0f - Tileable3dNoise::WorleyNoise(coord, cellCount * 1.0f);
                float w1 = 1.0f - Tileable3dNoise::WorleyNoise(coord, cellCount * 2.0f);
                float w2 = 1.0f - Tileable3dNoise::WorleyNoise(coord, cellCount * 4.0f);
                float w3 = 1.0f - Tileable3dNoise::WorleyNoise(coord, cellCount * 8.0f);

                float fbm0 = w0 * 0.625f + w1 * 0.25f + w2 * 0.125f;
                float fbm1 = w1 * 0.625f + w2 * 0.25f + w3 * 0.125f;
                float fbm2 = w2 * 0.75f  + w3 * 0.25f;

                size_t a = (size_t(z) * N + y) * N + x;
                data[a*4 + 0] = std::clamp(fbm0, 0.0f, 1.0f);
                data[a*4 + 1] = std::clamp(fbm1, 0.0f, 1.0f);
                data[a*4 + 2] = std::clamp(fbm2, 0.0f, 1.0f);
                data[a*4 + 3] = 1.0f;
            }
        }
    }
    return data;
}
