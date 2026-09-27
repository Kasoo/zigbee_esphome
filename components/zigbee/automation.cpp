#include "automation.h"
#include <algorithm>
#include "esphome/core/log.h"

namespace esphome {
namespace zigbee {

/**
 * @brief sRGB to R'G'B' gamma correction as outlined in
 *        https://en.wikipedia.org/wiki/SRGB
 *
 * @param linear a component value in sRGB color space
 * @return float the corresponding component value in R'G'B' color space
 */
inline float gamma_correct(float linear) {
  if(linear < 0.0031308f) {
    return linear * 12.92f;
  } else {
    return (1.055f) * pow(linear, (1.0f / 2.4f)) - 0.055f;
  }
}

/* The following conversions operate on xyY with Y = 1.0, hence Y
   doesn't appear in the formulas. The coefficients originate from
   https://en.wikipedia.org/wiki/SRGB#Primaries
 */
float get_r_from_xy(float x, float y) {
  float z = 1.0f - x - y;
  float X = x / y;
  float Z = z / y;
  float r = X * 3.2406 - 1.5372f - Z * 0.4986f;
  return std::clamp(gamma_correct(r), 0.0f, 1.0f);
}

float get_g_from_xy(float x, float y) {
  float z = 1.0f - x - y;
  float X = x / y;
  float Z = z / y;
  float g = -X * 0.9689f + 1.8758f + Z * 0.0415f;
  return std::clamp(gamma_correct(g), 0.0f, 1.0f);
}

float get_b_from_xy(float x, float y) {
  float z = 1.0f - x - y;
  float X = x / y;
  float Z = z / y;

  float b = X * 0.0557f - 0.2040f + Z * 1.0570f;
  return std::clamp(gamma_correct(b), 0.0f, 1.0f);
}

/// Inverse of the above: R'G'B' to CIE xy.
void get_xy_from_rgb(float r, float g, float b, float *x, float *y) {
  auto linearize = [](float c) { return c <= 0.04045f ? c / 12.92f : powf((c + 0.055f) / 1.055f, 2.4f); };
  r = linearize(r);
  g = linearize(g);
  b = linearize(b);
  float X = r * 0.4124f + g * 0.3576f + b * 0.1805f;
  float Y = r * 0.2126f + g * 0.7152f + b * 0.0722f;
  float Z = r * 0.0193f + g * 0.1192f + b * 0.9505f;
  float sum = X + Y + Z;
  if (sum <= 0.0f) {  // black has no chromaticity; use the D65 white point
    *x = 0.3127f;
    *y = 0.3290f;
    return;
  }
  *x = X / sum;
  *y = Y / sum;
}

#ifdef USE_LIGHT
void set_light_color(uint8_t ep, light::LightCall *call, uint16_t value, bool is_x) {
  static std::map<uint8_t, float> x;
  static std::map<uint8_t, float> y;
  if (is_x) {
    x[ep] = (float) value / 65536;
  } else {
    y[ep] = (float) value / 65536;
  }
  ESP_LOGD(TAG, "Set color, x: %f, y: %f", x[ep], y[ep]);
  call->set_rgb(get_r_from_xy(x[ep], y[ep]), get_g_from_xy(x[ep], y[ep]), get_b_from_xy(x[ep], y[ep]));
}
#endif

}  // namespace zigbee
}  // namespace esphome
