#pragma once 

#include <vector>
#include <cstdint>


/*
offset size   content
0      4      magic = "CVOL"
4      4      version = 1 (uint32)
8      4      width   N (uint32)
12     4      height  N (uint32)
16     4      depth   N (uint32)
20     4      channels (uint32) — 4 for RGBA
24     4      dtype   (uint32) — 0 = FLOAT32
28     ...    payload: w*h*d*channels * sizeof(float) bytes

*/ 
bool saveVolume3D(const char* path, int N, int channels, const std::vector<float>& data);

bool loadVolume3D(const char* path, int& N, int& channels, std::vector<float>& data);
