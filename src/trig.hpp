// The six trigonometric functions, their metadata, and safe evaluation.
#pragma once

#include <cmath>
#include <cstring>
#include <optional>
#include <string>

#include "parse.hpp"

namespace trigviz {

// y = amplitude * f(frequency * x + phase) + offset
struct Transform {
  double amplitude = 1.0;
  double frequency = 1.0;
  double phase = 0.0;
  double offset = 0.0;
};

struct TrigFunction {
  char key;
  const char *name;
  const char *ratio;
  double (*fn)(double);
  double (*denom)(double); // nullptr when defined everywhere
  bool bounded;
  int color; // ANSI bright color code (91-96)
};

namespace detail {
inline double Sin(double a) { return std::sin(a); }
inline double Cos(double a) { return std::cos(a); }
inline double Tan(double a) { return std::tan(a); }
inline double Csc(double a) { return 1.0 / std::sin(a); }
inline double Sec(double a) { return 1.0 / std::cos(a); }
inline double Cot(double a) { return std::cos(a) / std::sin(a); }
} // namespace detail

inline const TrigFunction kFunctions[] = {
    {'s', "sine", "opposite / hypotenuse", detail::Sin, nullptr, true, 96},
    {'c', "cosine", "adjacent / hypotenuse", detail::Cos, nullptr, true, 95},
    {'t', "tangent", "opposite / adjacent", detail::Tan, detail::Cos, false, 93},
    {'C', "cosecant", "hypotenuse / opposite", detail::Csc, detail::Sin, false, 92},
    {'S', "secant", "hypotenuse / adjacent", detail::Sec, detail::Cos, false, 91},
    {'T', "cotangent", "adjacent / opposite", detail::Cot, detail::Sin, false, 94},
};

inline const TrigFunction *FindByKey(char key) {
  for (const auto &f : kFunctions)
    if (f.key == key)
      return &f;
  return nullptr;
}

// Accepts short and long names: "sin", "sine", "cos", "cosine", ...
inline const TrigFunction *FindByName(const std::string &name) {
  static const struct {
    const char *alias;
    char key;
  } kAliases[] = {
      {"sin", 's'}, {"sine", 's'},      {"cos", 'c'}, {"cosine", 'c'},
      {"tan", 't'}, {"tangent", 't'},   {"csc", 'C'}, {"cosecant", 'C'},
      {"sec", 'S'}, {"secant", 'S'},    {"cot", 'T'}, {"cotangent", 'T'},
  };
  std::string lower;
  for (char ch : name)
    lower += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  for (const auto &a : kAliases)
    if (lower == a.alias)
      return FindByKey(a.key);
  return nullptr;
}

// Evaluates f at x under the given transform. Returns nullopt at an
// asymptote (the underlying denominator vanishes) so callers can skip
// the point instead of plotting garbage.
inline std::optional<double> Evaluate(const TrigFunction &f, double x,
                                      const Transform &t = {}) {
  double arg = t.frequency * x + t.phase;
  if (f.denom && std::fabs(f.denom(arg)) < 1e-9)
    return std::nullopt;
  return t.amplitude * f.fn(arg) + t.offset;
}

} // namespace trigviz
