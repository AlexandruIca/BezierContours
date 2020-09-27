#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <vector>

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

#include "stb_image_write.h"

template<typename T>
auto clamp(T const x, T const a, T const b) -> T
{
    return std::min(std::max(x, a), b);
}

struct point
{
    float x;
    float y;
};

struct curve
{
    point p1;
    point p2;
    point p3;
};

[[nodiscard]] auto curve_str(curve const& c) -> std::string
{
    return fmt::format("({}, {}), ({}, {}), ({}, {})", c.p1.x, c.p1.y, c.p2.x, c.p2.y, c.p3.x, c.p3.y);
}

auto eval_curve(float const y1, float const y2, float const y3, float const t) -> float
{
    float const it = 1.0F - t;

    return it * it * y1 + 2.0F * t * it * y2 + t * t * y3;
}

int width = 800;
int height = 800;
int num_channels = 4;

enum class orientation
{
    horizontal,
    vertical
};

auto trace_ray(std::vector<curve> const& curves,
               float const fx,
               float const fy,
               float const ppem,
               orientation const orient = orientation::horizontal) -> float
{
    float coverage = 0.0F;

    for(auto const& crv : curves) {
        auto x1 = crv.p1.x - fx;
        auto x2 = crv.p2.x - fx;
        auto x3 = crv.p3.x - fx;

        auto y1 = crv.p1.y - fy;
        auto y2 = crv.p2.y - fy;
        auto y3 = crv.p3.y - fy;

        if(orient == orientation::vertical) {
            x1 = crv.p1.y - fy;
            x2 = crv.p2.y - fy;
            x3 = crv.p3.y - fy;

            y1 = crv.p1.x - fx;
            y2 = crv.p2.x - fx;
            y3 = crv.p3.x - fx;
        }

        auto const a = y1 - 2 * y2 + y3;
        auto const b = y1 - y2;
        auto const c = y1;

        float t1 = 0.0F;
        float t2 = 0.0F;

        if(std::abs(a) < 0.0001F) {
            t1 = t2 = c / (2.0F * b);
        }
        else {
            float const root = std::sqrt(std::max(b * b - a * c, 0.0F));
            t1 = (b - root) / a;
            t2 = (b + root) / a;
        }

        auto const num = ((y1 > 0.0F) ? 2 : 0) + ((y2 > 0.0F) ? 4 : 0) + ((y3 > 0.0F) ? 8 : 0);
        auto const sh = 0x2E74 >> num;

        if((sh & 1) != 0) {
            float const r1 = eval_curve(x1, x2, x3, t1);
            coverage += clamp(r1 * ppem + 0.5F, 0.0F, 1.0F);
        }
        if((sh & 2) != 0) {
            float const r2 = eval_curve(x1, x2, x3, t2);
            coverage -= clamp(r2 * ppem + 0.5F, 0.0F, 1.0F);
        }
    }

    return coverage;
}

std::function<int(FT_Vector const*, void*)> g_move_to;
std::function<int(FT_Vector const*, void*)> g_line_to;
std::function<int(FT_Vector const*, FT_Vector const*, void*)> g_quadratic_to;
std::function<int(FT_Vector const*, FT_Vector const*, FT_Vector const*, void*)> g_cubic_to;

auto main() noexcept -> int
{
    FT_Library library;
    FT_Face face;

    auto error = FT_Init_FreeType(&library);

    if(error) {
        spdlog::error("Couldn't initialize Freetype!");
    }

    error = FT_New_Face(library, "./JFWilwod.ttf", 0, &face);

    if(error == FT_Err_Unknown_File_Format) {
        spdlog::error("Font file not recognized by Freetype!");
    }
    else if(error) {
        spdlog::error("Font file could not be read :(");
    }

    auto const em_units = face->units_per_EM;

    spdlog::info("num_glyphs: {}", face->num_glyphs);
    spdlog::info("units_per_em: {}", em_units);

    FT_ULong const index = 87;

    spdlog::info("Outline data for glyph #{}", index);

    FT_UInt glyph_index = FT_Get_Char_Index(face, index);
    if(FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_SCALE)) {
        spdlog::error("Could not load glyph #{}", index);
    }

    FT_Pos glyph_width = face->glyph->metrics.width;
    FT_Pos glyph_height = face->glyph->metrics.height;

    spdlog::info("Glyph metrics: w={}, h={}", glyph_width, glyph_height);

    std::vector<curve> curves;

    point prev{ 0.0F, 0.0F };
    float min_x = 16384.0F;
    float min_y = 16384.0F;
    float max_x = -16384.0F;
    float max_y = -16384.0F;

    g_move_to = [&](FT_Vector const* to, void*) -> int {
        spdlog::info("Move to: ({}, {})", to->x, to->y);
        prev = point{ static_cast<float>(to->x), static_cast<float>(to->y) };
        min_x = std::min(min_x, prev.x);
        min_y = std::min(min_y, prev.y);
        max_x = std::max(max_x, prev.x);
        max_y = std::max(max_y, prev.y);
        return 0;
    };
    g_line_to = [&](FT_Vector const* to, void*) -> int {
        spdlog::info("Line to: ({}, {})", to->x, to->y);
        point const c{ static_cast<float>(to->x), static_cast<float>(to->y) };
        curves.push_back(curve{ prev, point{ (prev.x + c.x) / 2.0F, (prev.y + c.y) / 2.0F }, c });
        prev = c;
        min_x = std::min(min_x, prev.x);
        min_y = std::min(min_y, prev.y);
        max_x = std::max(max_x, prev.x);
        max_y = std::max(max_y, prev.y);
        return 0;
    };
    g_quadratic_to = [&](FT_Vector const* control, FT_Vector const* to, void*) -> int {
        spdlog::info("Quadratic to ({}, {}), ({}, {})", control->x, control->y, to->x, to->y);
        curves.push_back(curve{ prev,
                                point{ static_cast<float>(control->x), static_cast<float>(control->y) },
                                point{ static_cast<float>(to->x), static_cast<float>(to->y) } });
        prev = point{ static_cast<float>(to->x), static_cast<float>(to->y) };
        min_x = std::min({ min_x, static_cast<float>(to->x), static_cast<float>(control->x) });
        min_y = std::min({ min_y, static_cast<float>(to->y), static_cast<float>(control->y) });
        max_x = std::max({ max_x, static_cast<float>(to->x), static_cast<float>(control->x) });
        max_y = std::max({ max_y, static_cast<float>(to->y), static_cast<float>(control->y) });
        return 0;
    };
    g_cubic_to = [&](FT_Vector const* control1, FT_Vector const* control2, FT_Vector const* to, void*) -> int {
        spdlog::info(
            "Cubic to ({}, {}), ({}, {}), ({}, {})", control1->x, control1->y, control2->x, control2->y, to->x, to->y);
        return 0;
    };

    FT_Outline_Funcs f;

    f.delta = 0;
    f.shift = 0;

    f.move_to = [](FT_Vector const* to, void*) -> int { return g_move_to(to, nullptr); };
    f.line_to = [](FT_Vector const* to, void*) -> int { return g_line_to(to, nullptr); };
    f.conic_to = [](FT_Vector const* control, FT_Vector const* to, void*) -> int {
        return g_quadratic_to(control, to, nullptr);
    };
    f.cubic_to = [](FT_Vector const* c1, FT_Vector const* c2, FT_Vector const* to, void*) -> int {
        return g_cubic_to(c1, c2, to, nullptr);
    };

    error = FT_Outline_Decompose(&face->glyph->outline, &f, nullptr);

    if(error) {
        spdlog::error("Could not decompose outlines!");
    }

    for(auto const& c : curves) {
        spdlog::info("Draw quadratic: {}", curve_str(c));
    }

    spdlog::info("MinX={}, MinY={}", min_x, min_y);
    spdlog::info("MaxX={}, MaxY={}", max_x, max_y);

    width = static_cast<int>(max_x) - static_cast<int>(min_x);
    height = static_cast<int>(max_y) - static_cast<int>(min_y);

    spdlog::info("w={}, h={}", width, height);

    std::vector<std::uint8_t> pixels;
    pixels.resize(width * height * num_channels);

    for(int y = 0; y < height; ++y) {
        for(int x = 0; x < width; ++x) {
            auto const fx = float(x) + min_x;
            auto const fy = float(y) + min_y;

            float const ppem_h = width * 1.0F;
            float const ppem_v = height * 1.0F;

            float const coverage_h = std::min(std::abs(trace_ray(curves, fx, fy, ppem_h)), 1.0F);
            float const coverage_v = std::min(std::abs(trace_ray(curves, fx, fy, ppem_v, orientation::vertical)), 1.0F);
            float const avg_coverage = (coverage_h + coverage_v) / 2.0F;

            auto const max_height = (height - 1) * width * num_channels;

            pixels[max_height - y * width * num_channels + x * num_channels] = 255 * avg_coverage;
            pixels[max_height - y * width * num_channels + x * num_channels + 1] = 128 * avg_coverage;
            pixels[max_height - y * width * num_channels + x * num_channels + 2] = 64 * avg_coverage;
            pixels[max_height - y * width * num_channels + x * num_channels + 3] = 255;
        }
    }

    stbi_write_png("img.png", width, height, num_channels, pixels.data(), width * num_channels);
}
