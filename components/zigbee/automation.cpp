#include "automation.h"
#include <algorithm>
#include "esphome/core/log.h"

namespace esphome {
namespace zigbee {

/// CIE xy (Y = 1) to linear RGB, sRGB primaries, scaled so the largest channel is 1.
/// Linear because the LED's output is linear in the value it is sent; any curve is left to the light's
/// gamma_correct. Scaling before clamping keeps the channel ratios of saturated colours.
void get_rgb_from_xy(float x, float y, float *r, float *g, float *b) {
  if (y <= 0.0f) {
    *r = *g = *b = 1.0f;
    return;
  }
  float X = x / y;
  float Z = (1.0f - x - y) / y;
  float lr = std::max(X * 3.2406f - 1.5372f - Z * 0.4986f, 0.0f);
  float lg = std::max(-X * 0.9689f + 1.8758f + Z * 0.0415f, 0.0f);
  float lb = std::max(X * 0.0557f - 0.2040f + Z * 1.0570f, 0.0f);
  float m = std::max({lr, lg, lb});
  if (m <= 0.0f) {
    *r = *g = *b = 1.0f;
    return;
  }
  *r = lr / m;
  *g = lg / m;
  *b = lb / m;
}

/// Inverse of the above: linear RGB to CIE xy.
void get_xy_from_rgb(float r, float g, float b, float *x, float *y) {
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
  float r, g, b;
  get_rgb_from_xy(x[ep], y[ep], &r, &g, &b);
  call->set_rgb(r, g, b);
}
#endif

}  // namespace zigbee
}  // namespace esphome
