#include "Color.h"
#include <cmath>

int HSBtoRGB(float hue, float saturation, float brightness)
{
    float h = std::clamp(hue, 0.0f, 1.0f);
    float s = std::clamp(saturation, 0.0f, 1.0f);
    float v = std::clamp(brightness, 0.0f, 1.0f);

    int r = 0, g = 0, b = 0;

    if (s == 0.0f)
    {
        r = g = b = static_cast<int>(v * 255.0f + 0.5f);
    }
    else
    {
        // Java takes hue as 0.0 to 1.0, map it to 6 sectors
        float h_scaled = (h - std::floor(h)) * 6.0f;
        int i = static_cast<int>(std::floor(h_scaled));
        float f = h_scaled - i;

        float p = v * (1.0f - s);
        float q = v * (1.0f - s * f);
        float t = v * (1.0f - s * (1.0f - f));

        switch (i)
        {
        case 0:
            r = static_cast<int>(v * 255.0f + 0.5f);
            g = static_cast<int>(t * 255.0f + 0.5f);
            b = static_cast<int>(p * 255.0f + 0.5f);
            break;
        case 1:
            r = static_cast<int>(q * 255.0f + 0.5f);
            g = static_cast<int>(v * 255.0f + 0.5f);
            b = static_cast<int>(p * 255.0f + 0.5f);
            break;
        case 2:
            r = static_cast<int>(p * 255.0f + 0.5f);
            g = static_cast<int>(v * 255.0f + 0.5f);
            b = static_cast<int>(t * 255.0f + 0.5f);
            break;
        case 3:
            r = static_cast<int>(p * 255.0f + 0.5f);
            g = static_cast<int>(q * 255.0f + 0.5f);
            b = static_cast<int>(v * 255.0f + 0.5f);
            break;
        case 4:
            r = static_cast<int>(t * 255.0f + 0.5f);
            g = static_cast<int>(p * 255.0f + 0.5f);
            b = static_cast<int>(v * 255.0f + 0.5f);
            break;
        case 5:
            r = static_cast<int>(v * 255.0f + 0.5f);
            g = static_cast<int>(p * 255.0f + 0.5f);
            b = static_cast<int>(q * 255.0f + 0.5f);
            break;
        }
    }

    // Pack channels exactly like Java: 0xAARRGGBB (Alpha is fully opaque 0xFF)
    return (0xFF << 24) | (r << 16) | (g << 8) | b;
}
