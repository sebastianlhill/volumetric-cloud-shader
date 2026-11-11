#include "cloudbake.h"
#include "volumeio.h"
#include <cstdio>

int main(int argc, char** argv) {
    const char* shape_path   = (argc > 1) ? argv[1] : "data/cloudshape.vol";
    const char* erosion_path = (argc > 2) ? argv[2] : "data/clouderosion.vol";

    std::printf("Baking cloud shape (128^3)...\n");
    auto shape = bakeCloudShape(128);
    if (!saveVolume3D(shape_path, 128, 4, shape)) {
        std::fprintf(stderr, "Failed to write %s\n", shape_path);
        return 1;
    }

    std::printf("Baking cloud erosion (32^3)...\n");
    auto erosion = bakeCloudErosion(32);
    if (!saveVolume3D(erosion_path, 32, 4, erosion)) {
        std::fprintf(stderr, "Failed to write %s\n", erosion_path);
        return 1;
    }

    std::printf("Wrote %s and %s\n", shape_path, erosion_path);
    return 0;
}
