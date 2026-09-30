// Parsing and formatting helpers for angles expressed in terms of pi.
#pragma once

#include <cctype>
#include <cmath>
#include <cstdio>
#include <numeric>
#include <optional>
#include <string>

namespace trigviz {

inline constexpr double kPi = 3.14159265358979323846;

// Parses a plain number or a multiple of pi:
//   "1.5"  "-2"  "pi"  "-pi/2"  "2pi"  "3pi/4"  "0.5pi"  "π"
inline std::optional<double> ParseAngle(const std::string &input) {
  // Normalize: replace UTF-8 "π" (0xCF 0x80) with "pi", strip spaces, lowercase.
  std::string s;
  for (size_t i = 0; i < input.size(); ++i) {
    unsigned char c = static_cast<unsigned char>(input[i]);
    if (c == 0xCF && i + 1 < input.size() &&
        static_cast<unsigned char>(input[i + 1]) == 0x80) {
      s += "pi";
      ++i;
    } else if (!std::isspace(c)) {
      s += static_cast<char>(std::tolower(c));
    }
  }
  if (s.empty())
    return std::nullopt;

  double sign = 1.0;
  size_t pos = 0;
  if (s[0] == '+' || s[0] == '-') {
    sign = (s[0] == '-') ? -1.0 : 1.0;
    pos = 1;
  }
  if (pos >= s.size())
    return std::nullopt;

  size_t piPos = s.find("pi", pos);
  if (piPos == std::string::npos) {
    try {
      size_t used = 0;
      double v = std::stod(s.substr(pos), &used);
      if (used != s.size() - pos)
        return std::nullopt;
      return sign * v;
    } catch (...) {
      return std::nullopt;
    }
  }

  double coef = 1.0;
  if (piPos > pos) {
    std::string coefStr = s.substr(pos, piPos - pos);
    if (coefStr.back() == '*')
      coefStr.pop_back();
    if (!coefStr.empty()) {
      try {
        size_t used = 0;
        coef = std::stod(coefStr, &used);
        if (used != coefStr.size())
          return std::nullopt;
      } catch (...) {
        return std::nullopt;
      }
    }
  }

  double denom = 1.0;
  size_t rest = piPos + 2;
  if (rest < s.size()) {
    if (s[rest] != '/' || rest + 1 >= s.size())
      return std::nullopt;
    try {
      size_t used = 0;
      denom = std::stod(s.substr(rest + 1), &used);
      if (used != s.size() - rest - 1)
        return std::nullopt;
    } catch (...) {
      return std::nullopt;
    }
    if (denom == 0.0)
      return std::nullopt;
  }

  return sign * coef * kPi / denom;
}

// Formats (num/den) * pi as a compact label, e.g. (3, 4) -> "3π/4",
// (8, 8) -> "π", (0, 4) -> "0". Uses "pi" instead of "π" when asciiPi is set.
inline std::string FormatPiFraction(int num, int den, bool asciiPi = false) {
  if (num == 0)
    return "0";
  int g = std::gcd(num < 0 ? -num : num, den);
  num /= g;
  den /= g;
  const char *pi = asciiPi ? "pi" : "\xCF\x80";
  std::string out;
  if (num == -1)
    out = "-";
  else if (num != 1)
    out = std::to_string(num);
  out += pi;
  if (den != 1) {
    out += '/';
    out += std::to_string(den);
  }
  return out;
}

// Compact numeric label: trims trailing zeros ("2", "0.5", "2.5").
inline std::string FormatNumber(double v) {
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%g", v);
  return buf;
}

// Smallest "nice" step (1, 2, 2.5, or 5 times a power of ten) >= target.
inline double NiceStep(double target) {
  if (target <= 0)
    return 1.0;
  double mag = std::pow(10.0, std::floor(std::log10(target)));
  for (double m : {1.0, 2.0, 2.5, 5.0, 10.0}) {
    if (mag * m >= target - 1e-12)
      return mag * m;
  }
  return 10.0 * mag;
}

} // namespace trigviz
