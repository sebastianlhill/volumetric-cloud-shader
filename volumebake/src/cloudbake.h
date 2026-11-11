#pragma once 
#include <vector>

    std::vector<float> bakeCloudShape(int N);

inline static float remap(float v, float a, float b, float c, float d){
  return c + ((v-a) / (v-b)) * (d-c);
}
    std::vector<float> bakeCloudErosion(int N);
