/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Assertions.h>
#include <AK/Format.h>
#include <AK/Forward.h>
#include <AK/SIMD.h>
#include <AK/StdLibExtras.h>
#include <LibIPC/Forward.h>
#include <math.h>

namespace Gfx {

enum class ColorRole;
typedef u32 RGBA32;

constexpr u32 make_rgb(u8 r, u8 g, u8 b)
{
    return ((r << 16) | (g << 8) | b);
}

struct HSV {
    double hue { 0 };
    double saturation { 0 };
    double value { 0 };
};

class Color {
public:
    enum NamedColor {
        Transparent,
        Black,
        White,
        Red,
        Green,
        Cyan,
        Blue,
        Yellow,
        Magenta,
        DarkGray,
        MidGray,
        LightGray,
        WarmGray,
        DarkCyan,
        DarkGreen,
        DarkBlue,
        DarkRed,
        MidCyan,
        MidGreen,
        MidRed,
        MidBlue,
        MidMagenta,
    };

    constexpr Color() { }
    constexpr Color(NamedColor);
    constexpr Color(u8 r, u8 g, u8 b)
        : m_value(0xff000000 | (r << 16) | (g << 8) | b)
    {
    }
    constexpr Color(u8 r, u8 g, u8 b, u8 a)
        : m_value((a << 24) | (r << 16) | (g << 8) | b)
    {
    }

    static constexpr Color from_rgb(unsigned rgb) { return Color(rgb | 0xff000000); }
    static constexpr Color from_rgba(unsigned rgba) { return Color(rgba); }

    static constexpr Color from_cmyk(float c, float m, float y, float k)
    {
        auto r = static_cast<u8>(255.0f * (1.0f - c) * (1.0f - k));
        auto g = static_cast<u8>(255.0f * (1.0f - m) * (1.0f - k));
        auto b = static_cast<u8>(255.0f * (1.0f - y) * (1.0f - k));

        return Color(r, g, b);
    }

    constexpr u8 red() const { return (m_value >> 16) & 0xff; }
    constexpr u8 green() const { return (m_value >> 8) & 0xff; }
    constexpr u8 blue() const { return m_value & 0xff; }
    constexpr u8 alpha() const { return (m_value >> 24) & 0xff; }

    void set_alpha(u8 value)
    {
        m_value &= 0x00ffffff;
        m_value |= value << 24;
    }

    constexpr void set_red(u8 value)
    {
        m_value &= 0xff00ffff;
        m_value |= value << 16;
    }

    constexpr void set_green(u8 value)
    {
        m_value &= 0xffff00ff;
        m_value |= value << 8;
    }

    constexpr void set_blue(u8 value)
    {
        m_value &= 0xffffff00;
        m_value |= value;
    }

    constexpr Color with_alpha(u8 alpha) const
    {
        return Color((m_value & 0x00ffffff) | alpha << 24);
    }

    constexpr Color blend(Color source) const
    {
        if (!alpha() || source.alpha() == 255)
            return source;

        if (!source.alpha())
            return *this;

#ifdef __SSE__
        using AK::SIMD::i32x4;

        const i32x4 color = {
            red(),
            green(),
            blue()
        };
        const i32x4 source_color = {
            source.red(),
            source.green(),
            source.blue()
        };

        const int d = 255 * (alpha() + source.alpha()) - alpha() * source.alpha();
        const i32x4 out = (color * alpha() * (255 - source.alpha()) + 255 * source.alpha() * source_color) / d;
        return Color(out[0], out[1], out[2], d / 255);
#else
        int d = 255 * (alpha() + source.alpha()) - alpha() * source.alpha();
        u8 r = (red() * alpha() * (255 - source.alpha()) + 255 * source.alpha() * source.red()) / d;
        u8 g = (green() * alpha() * (255 - source.alpha()) + 255 * source.alpha() * source.green()) / d;
        u8 b = (blue() * alpha() * (255 - source.alpha()) + 255 * source.alpha() * source.blue()) / d;
        u8 a = d / 255;
        return Color(r, g, b, a);
#endif
    }

    constexpr Color interpolate(const Color& other, float weight) const
    {
        u8 r = red() + roundf(static_cast<float>(other.red() - red()) * weight);
        u8 g = green() + roundf(static_cast<float>(other.green() - green()) * weight);
        u8 b = blue() + roundf(static_cast<float>(other.blue() - blue()) * weight);
        u8 a = alpha() + roundf(static_cast<float>(other.alpha() - alpha()) * weight);
        return Color(r, g, b, a);
    }

    constexpr Color multiply(const Color& other) const
    {
        return Color(
            red() * other.red() / 255,
            green() * other.green() / 255,
            blue() * other.blue() / 255,
            alpha() * other.alpha() / 255);
    }

    constexpr Color to_grayscale() const
    {
        int gray = (red() + green() + blue()) / 3;
        return Color(gray, gray, gray, alpha());
    }

    constexpr Color darkened(float amount = 0.5f) const
    {
        return Color(red() * amount, green() * amount, blue() * amount, alpha());
    }

    constexpr Color lightened(float amount = 1.2f) const
    {
        return Color(min(255, (int)((float)red() * amount)), min(255, (int)((float)green() * amount)), min(255, (int)((float)blue() * amount)), alpha());
    }

    constexpr Color inverted() const
    {
        return Color(~red(), ~green(), ~blue(), alpha());
    }

    constexpr Color xored(const Color& other) const
    {
        return Color(((other.m_value ^ m_value) & 0x00ffffff) | (m_value & 0xff000000));
    }

    constexpr RGBA32 value() const { return m_value; }

    constexpr bool operator==(const Color& other) const
    {
        return m_value == other.m_value;
    }

    constexpr bool operator!=(const Color& other) const
    {
        return m_value != other.m_value;
    }

    String to_string() const;
    String to_string_without_alpha() const;
    static Optional<Color> from_string(const StringView&);

    constexpr HSV to_hsv() const
    {
        HSV hsv;
        double r = static_cast<double>(red()) / 255.0;
        double g = static_cast<double>(green()) / 255.0;
        double b = static_cast<double>(blue()) / 255.0;
        double max = AK::max(AK::max(r, g), b);
        double min = AK::min(AK::min(r, g), b);
        double chroma = max - min;

        if (!chroma)
            hsv.hue = 0.0;
        else if (max == r)
            hsv.hue = (60.0 * ((g - b) / chroma)) + 360.0;
        else if (max == g)
            hsv.hue = (60.0 * ((b - r) / chroma)) + 120.0;
        else
            hsv.hue = (60.0 * ((r - g) / chroma)) + 240.0;

        if (hsv.hue >= 360.0)
            hsv.hue -= 360.0;

        if (!max)
            hsv.saturation = 0;
        else
            hsv.saturation = chroma / max;

        hsv.value = max;

        VERIFY(hsv.hue >= 0.0 && hsv.hue < 360.0);
        VERIFY(hsv.saturation >= 0.0 && hsv.saturation <= 1.0);
        VERIFY(hsv.value >= 0.0 && hsv.value <= 1.0);

        return hsv;
    }

    static constexpr Color from_hsv(double hue, double saturation, double value)
    {
        return from_hsv({ hue, saturation, value });
    }

    static constexpr Color from_hsv(const HSV& hsv)
    {
        VERIFY(hsv.hue >= 0.0 && hsv.hue < 360.0);
        VERIFY(hsv.saturation >= 0.0 && hsv.saturation <= 1.0);
        VERIFY(hsv.value >= 0.0 && hsv.value <= 1.0);

        double hue = hsv.hue;
        double saturation = hsv.saturation;
        double value = hsv.value;

        int high = static_cast<int>(hue / 60.0) % 6;
        double f = (hue / 60.0) - high;
        double c1 = value * (1.0 - saturation);
        double c2 = value * (1.0 - saturation * f);
        double c3 = value * (1.0 - saturation * (1.0 - f));

        double r = 0;
        double g = 0;
        double b = 0;

        switch (high) {
        case 0:
            r = value;
            g = c3;
            b = c1;
            break;
        case 1:
            r = c2;
            g = value;
            b = c1;
            break;
        case 2:
            r = c1;
            g = value;
            b = c3;
            break;
        case 3:
            r = c1;
            g = c2;
            b = value;
            break;
        case 4:
            r = c3;
            g = c1;
            b = value;
            break;
        case 5:
            r = value;
            g = c1;
            b = c2;
            break;
        }

        u8 out_r = (u8)(r * 255);
        u8 out_g = (u8)(g * 255);
        u8 out_b = (u8)(b * 255);
        return Color(out_r, out_g, out_b);
    }

private:
    constexpr explicit Color(RGBA32 rgba)
        : m_value(rgba)
    {
    }

    RGBA32 m_value { 0 };
};

constexpr Color::Color(NamedColor named)
{
    if (named == Transparent) {
        m_value = 0;
        return;
    }

    struct {
        u8 r;
        u8 g;
        u8 b;
    } rgb;

    switch (named) {
    case Black:
        rgb = { 0, 0, 0 };
        break;
    case White:
        rgb = { 255, 255, 255 };
        break;
    case Red:
        rgb = { 255, 0, 0 };
        break;
    case Green:
        rgb = { 0, 255, 0 };
        break;
    case Cyan:
        rgb = { 0, 255, 255 };
        break;
    case DarkCyan:
        rgb = { 0, 127, 127 };
        break;
    case MidCyan:
        rgb = { 0, 192, 192 };
        break;
    case Blue:
        rgb = { 0, 0, 255 };
        break;
    case Yellow:
        rgb = { 255, 255, 0 };
        break;
    case Magenta:
        rgb = { 255, 0, 255 };
        break;
    case DarkGray:
        rgb = { 64, 64, 64 };
        break;
    case MidGray:
        rgb = { 127, 127, 127 };
        break;
    case LightGray:
        rgb = { 192, 192, 192 };
        break;
    case MidGreen:
        rgb = { 0, 192, 0 };
        break;
    case MidBlue:
        rgb = { 0, 0, 192 };
        break;
    case MidRed:
        rgb = { 192, 0, 0 };
        break;
    case MidMagenta:
        rgb = { 192, 0, 192 };
        break;
    case DarkGreen:
        rgb = { 0, 128, 0 };
        break;
    case DarkBlue:
        rgb = { 0, 0, 128 };
        break;
    case DarkRed:
        rgb = { 128, 0, 0 };
        break;
    case WarmGray:
        rgb = { 212, 208, 200 };
        break;
    default:
        VERIFY_NOT_REACHED();
        break;
    }

    m_value = 0xff000000 | (rgb.r << 16) | (rgb.g << 8) | rgb.b;
}

}

using Gfx::Color;

namespace AK {

template<>
struct Formatter<Gfx::Color> : public Formatter<StringView> {
    void format(FormatBuilder& builder, const Gfx::Color& value);
};

}

namespace IPC {

bool encode(Encoder&, const Gfx::Color&);
bool decode(Decoder&, Gfx::Color&);

}
