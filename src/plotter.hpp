// Maps trig curves onto a Canvas: axes, pi-based tick labels, curves, legend.
#pragma once

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "canvas.hpp"
#include "parse.hpp"
#include "trig.hpp"

namespace trigviz {

struct PlotOptions {
  int width = 90;   // character cells
  int height = 28;  // character cells
  double xmin = -2 * kPi;
  double xmax = 2 * kPi;
  double ymax = 0; // 0 = pick automatically from the selected functions
  bool ascii = false;
  bool color = true;
  Transform transform;
};

namespace detail {

constexpr int kAxisColor = 90; // dim gray

inline double AutoYMax(const std::vector<const TrigFunction *> &funcs,
                       const Transform &t) {
  bool unbounded = false;
  for (const auto *f : funcs)
    if (!f->bounded)
      unbounded = true;
  double a = std::fabs(t.amplitude);
  double d = std::fabs(t.offset);
  double ymax = unbounded ? 4.0 * std::max(1.0, a) + d : a + d + 0.5;
  return std::max(ymax, 1.0);
}

} // namespace detail

// Renders the plot (axes + curves) followed by a legend line per function.
inline std::string RenderPlot(const std::vector<const TrigFunction *> &funcs,
                              const PlotOptions &opt) {
  Canvas canvas(opt.width, opt.height);
  const int dotW = canvas.DotWidth();
  const int dotH = canvas.DotHeight();
  const double xrange = opt.xmax - opt.xmin;
  const double ymax = opt.ymax > 0 ? opt.ymax
                                   : detail::AutoYMax(funcs, opt.transform);

  auto xToDot = [&](double x) {
    return static_cast<int>((x - opt.xmin) / xrange * dotW);
  };
  auto yToDot = [&](double y) {
    return static_cast<int>((ymax - y) / (2.0 * ymax) * dotH);
  };

  // Axes (weak text, so curves draw over them).
  const bool yAxisVisible = opt.xmin <= 0 && opt.xmax >= 0;
  const int yAxisCol = yAxisVisible
                           ? std::clamp(xToDot(0.0) / 2, 0, opt.width - 1)
                           : 0;
  const int xAxisRow = std::clamp(yToDot(0.0) / 4, 0, opt.height - 1);
  for (int row = 0; row < opt.height; ++row)
    canvas.SetText(yAxisCol, row, '|', false, detail::kAxisColor);
  for (int col = 0; col < opt.width; ++col)
    canvas.SetText(col, xAxisRow, '-', false, detail::kAxisColor);
  canvas.SetText(yAxisCol, xAxisRow, '+', false, detail::kAxisColor);

  // X ticks at multiples of pi/8 (doubling the step until <= 8 ticks fit).
  int stepEighths = 1;
  while (xrange / (stepEighths * kPi / 8.0) > 8.0)
    stepEighths *= 2;
  const double xstep = stepEighths * kPi / 8.0;
  const int labelRow = (xAxisRow + 1 < opt.height) ? xAxisRow + 1 : xAxisRow - 1;
  for (int n = static_cast<int>(std::ceil(opt.xmin / xstep - 1e-9));
       n <= static_cast<int>(std::floor(opt.xmax / xstep + 1e-9)); ++n) {
    if (n == 0)
      continue;
    int col = std::clamp(xToDot(n * xstep) / 2, 0, opt.width - 1);
    canvas.SetText(col, xAxisRow, '+', false, detail::kAxisColor);
    std::string label = FormatPiFraction(n * stepEighths, 8, opt.ascii);
    int cols = static_cast<int>(Canvas::SplitGlyphs(label).size());
    int start = std::clamp(col - cols / 2, 0, opt.width - cols);
    canvas.SetLabel(start, labelRow, label, detail::kAxisColor);
  }

  // Y ticks at "nice" intervals, labeled beside the y-axis.
  const double ystep = NiceStep(2.0 * ymax / 6.0);
  for (int n = static_cast<int>(std::ceil(-ymax / ystep));
       n <= static_cast<int>(std::floor(ymax / ystep)); ++n) {
    if (n == 0)
      continue;
    double y = n * ystep;
    int row = std::clamp(yToDot(y) / 4, 0, opt.height - 1);
    canvas.SetText(yAxisCol, row, '+', false, detail::kAxisColor);
    canvas.SetLabel(yAxisCol + 2, row, FormatNumber(y), detail::kAxisColor);
  }

  // Curves: one sample per dot column, connected vertically between
  // neighboring samples so curves read as lines rather than dotted points.
  for (const auto *f : funcs) {
    int prevRow = 0;
    double prevY = 0;
    bool prevValid = false;
    for (int dx = 0; dx < dotW; ++dx) {
      double x = opt.xmin + (dx + 0.5) * xrange / dotW;
      auto y = Evaluate(*f, x, opt.transform);
      bool valid = y.has_value() && std::fabs(*y) <= ymax;
      if (valid) {
        int dy = std::clamp(yToDot(*y), 0, dotH - 1);
        canvas.SetDot(dx, dy, f->color);
        // Bridge the vertical gap, but not across a jump discontinuity.
        if (prevValid && std::abs(dy - prevRow) > 1 &&
            std::fabs(*y - prevY) < ymax) {
          int lo = std::min(dy, prevRow), hi = std::max(dy, prevRow);
          for (int r = lo + 1; r < hi; ++r)
            canvas.SetDot(dx, r, f->color);
        }
        prevRow = dy;
        prevY = *y;
      }
      prevValid = valid;
    }
  }

  std::string out = canvas.Render(opt.color, opt.ascii);

  // Legend.
  out += '\n';
  for (const auto *f : funcs) {
    std::string swatch = opt.ascii ? "--" : "\xE2\x94\x80\xE2\x94\x80"; // ──
    out += "  ";
    if (opt.color)
      out += "\x1b[" + std::to_string(f->color) + "m";
    out += swatch + " " + f->name;
    if (opt.color)
      out += "\x1b[0m";
    out += "  (" + std::string(f->ratio) + ")\n";
  }
  return out;
}

} // namespace trigviz
