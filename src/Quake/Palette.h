#pragma once

#include <stdexcept>
#include <vector>

#include <glm/vec3.hpp>

using glm::vec3;
using std::runtime_error;
using std::vector;

struct PaletteColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct Palette {
    vector<PaletteColor> colors;

    Palette(FILE*, int32_t offset, int32_t size);
};
