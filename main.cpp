#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

#include "stb_image_write.h"

struct point {
    float x;
    float y;
};

struct curve {
    point p1;
    point p2;
    point p3;
};

auto eval_curve(float const y1, float const y2, float const y3, float const t)
    -> float {
    float const it = 1.0F - t;

    return it * it * y1 + 2.0F * t * it * y2 + t * t * y3;
}

constexpr int width = 1600;
constexpr int height = 1600;
constexpr int num_channels = 4;

auto main() noexcept -> int {
    std::vector<curve> curves;

    curves.push_back({{0.3F, 0.3F}, {0.5F, 0.5F}, {0.3F, 0.7F}});  // clockwise
    curves.push_back(
        {{0.3F, 0.7F}, {1.0F, 0.5F}, {0.3F, 0.3F}});  // counter-clockwise

    curves.push_back(
        {{0.9F, 0.3F}, {0.9F, 0.5F}, {0.9F, 0.7F}});  // clockwise vertical
    curves.push_back(
        {{0.9F, 0.7F}, {0.93F, 0.7F}, {0.95F, 0.7F}});  // clockwise horizontal
    curves.push_back({{0.95F, 0.7F},
                      {0.95F, 0.5F},
                      {0.95F, 0.3F}});  // counter-clockwise vertical
    curves.push_back({{0.95F, 0.3F},
                      {0.93F, 0.3F},
                      {0.9F, 0.3F}});  // counter-clockwise horizontal

    std::vector<std::uint8_t> pixels;
    pixels.reserve(width * height * num_channels);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int winding = 0;
            auto const fx = float(x) / float(width);
            auto const fy = float(y) / float(height);

            for (auto const& crv : curves) {
                auto const x1 = crv.p1.x - fx;
                auto const x2 = crv.p2.x - fx;
                auto const x3 = crv.p3.x - fx;

                auto const y1 = crv.p1.y - fy;
                auto const y2 = crv.p2.y - fy;
                auto const y3 = crv.p3.y - fy;

                auto const a = y1 - 2 * y2 + y3;
                auto const b = y1 - y2;
                auto const c = y1;

                float t1 = 0.0F;
                float t2 = 0.0F;

                if (std::abs(a) < 0.0001F) {
                    t1 = t2 = c / (2.0F * b);
                } else {
                    float const root = std::sqrt(std::max(b * b - a * c, 0.0F));
                    t1 = (b - root) / a;
                    t2 = (b + root) / a;
                }

                auto const num = ((y1 > 0.0F) ? 2 : 0) + ((y2 > 0.0F) ? 4 : 0) +
                                 ((y3 > 0.0F) ? 8 : 0);
                auto const sh = 0x2E74 >> num;

                if ((sh & 1) != 0) {
                    float const r1 = eval_curve(x1, x2, x3, t1);
                    winding += static_cast<int>(r1 >= 0);
                }
                if ((sh & 2) != 0) {
                    float const r2 = eval_curve(x1, x2, x3, t2);
                    winding -= static_cast<int>(r2 >= 0);
                }
            }
            if (winding != 0) {
                pixels.push_back(255);
                pixels.push_back(128);
                pixels.push_back(64);
                pixels.push_back(255);
            } else {
                pixels.push_back(0);
                pixels.push_back(0);
                pixels.push_back(0);
                pixels.push_back(255);
            }
        }
    }

    stbi_write_png("img.png", width, height, num_channels, pixels.data(),
                   width * num_channels);
}
