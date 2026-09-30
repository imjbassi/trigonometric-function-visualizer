// Unit tests for the visualizer's parsing, evaluation, and rendering.
// Plain assert-style checks; the process exits nonzero on any failure.

#include <cmath>
#include <iostream>
#include <string>

#include "../src/canvas.hpp"
#include "../src/parse.hpp"
#include "../src/plotter.hpp"
#include "../src/trig.hpp"

namespace {

int failures = 0;

#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << "  " << #cond     \
                << '\n';                                                       \
      ++failures;                                                              \
    }                                                                          \
  } while (0)

bool Near(double a, double b, double eps = 1e-9) {
  return std::fabs(a - b) < eps;
}

using namespace trigviz;

void TestParseAngle() {
  CHECK(Near(*ParseAngle("1.5"), 1.5));
  CHECK(Near(*ParseAngle("-2"), -2.0));
  CHECK(Near(*ParseAngle("pi"), kPi));
  CHECK(Near(*ParseAngle("-pi"), -kPi));
  CHECK(Near(*ParseAngle("2pi"), 2 * kPi));
  CHECK(Near(*ParseAngle("-pi/2"), -kPi / 2));
  CHECK(Near(*ParseAngle("3pi/4"), 3 * kPi / 4));
  CHECK(Near(*ParseAngle("0.5pi"), kPi / 2));
  CHECK(Near(*ParseAngle("2*pi"), 2 * kPi));
  CHECK(Near(*ParseAngle(" PI / 2 "), kPi / 2));
  CHECK(Near(*ParseAngle("\xCF\x80"), kPi)); // UTF-8 pi
  CHECK(!ParseAngle(""));
  CHECK(!ParseAngle("garbage"));
  CHECK(!ParseAngle("pi/0"));
  CHECK(!ParseAngle("1.5x"));
  CHECK(!ParseAngle("-"));
}

void TestFormatPiFraction() {
  CHECK(FormatPiFraction(0, 8, true) == "0");
  CHECK(FormatPiFraction(8, 8, true) == "pi");
  CHECK(FormatPiFraction(-8, 8, true) == "-pi");
  CHECK(FormatPiFraction(4, 8, true) == "pi/2");
  CHECK(FormatPiFraction(-4, 8, true) == "-pi/2");
  CHECK(FormatPiFraction(12, 8, true) == "3pi/2");
  CHECK(FormatPiFraction(16, 8, true) == "2pi");
  CHECK(FormatPiFraction(2, 8, true) == "pi/4");
  CHECK(FormatPiFraction(6, 8, true) == "3pi/4");
  CHECK(FormatPiFraction(8, 8, false) == "\xCF\x80");
}

void TestNiceStep() {
  CHECK(Near(NiceStep(0.9), 1.0));
  CHECK(Near(NiceStep(1.0), 1.0));
  CHECK(Near(NiceStep(1.4), 2.0));
  CHECK(Near(NiceStep(2.2), 2.5));
  CHECK(Near(NiceStep(3.0), 5.0));
  CHECK(Near(NiceStep(7.0), 10.0));
  CHECK(Near(NiceStep(0.3), 0.5));
}

void TestLookup() {
  CHECK(FindByKey('s') && std::string(FindByKey('s')->name) == "sine");
  CHECK(FindByKey('T') && std::string(FindByKey('T')->name) == "cotangent");
  CHECK(FindByKey('x') == nullptr);
  CHECK(FindByName("sin") == FindByKey('s'));
  CHECK(FindByName("COSINE") == FindByKey('c'));
  CHECK(FindByName("cot") == FindByKey('T'));
  CHECK(FindByName("nope") == nullptr);
}

void TestEvaluate() {
  const TrigFunction &sine = *FindByKey('s');
  const TrigFunction &tangent = *FindByKey('t');
  const TrigFunction &cosecant = *FindByKey('C');
  const TrigFunction &secant = *FindByKey('S');

  CHECK(Near(*Evaluate(sine, kPi / 2), 1.0));
  CHECK(Near(*Evaluate(tangent, kPi / 4), 1.0, 1e-6));
  CHECK(Near(*Evaluate(secant, 0.0), 1.0));
  CHECK(!Evaluate(tangent, kPi / 2));  // asymptote
  CHECK(!Evaluate(cosecant, 0.0));     // asymptote
  CHECK(!Evaluate(cosecant, kPi));     // asymptote

  Transform t;
  t.amplitude = 2.0;
  t.offset = 1.0;
  CHECK(Near(*Evaluate(sine, kPi / 2, t), 3.0)); // 2*sin(pi/2) + 1

  Transform freq;
  freq.frequency = 2.0;
  CHECK(Near(*Evaluate(sine, kPi / 4, freq), 1.0)); // sin(2 * pi/4)

  Transform phase;
  phase.phase = kPi / 2;
  CHECK(Near(*Evaluate(sine, 0.0, phase), 1.0)); // sin(0 + pi/2)

  // The asymptote check applies to the transformed argument.
  Transform shiftIntoAsymptote;
  shiftIntoAsymptote.phase = kPi / 2;
  CHECK(!Evaluate(tangent, 0.0, shiftIntoAsymptote));
}

void TestCanvas() {
  {
    Canvas c(1, 1);
    c.SetDot(0, 0, -1);
    CHECK(c.Render(false, false) == "\xE2\xA0\x81\n"); // U+2801
  }
  {
    Canvas c(1, 1);
    for (int dx = 0; dx < 2; ++dx)
      for (int dy = 0; dy < 4; ++dy)
        c.SetDot(dx, dy, -1);
    CHECK(c.Render(false, false) == "\xE2\xA3\xBF\n"); // U+28FF, full cell
  }
  {
    Canvas c(1, 1);
    c.SetDot(0, 0, -1); // top dot only
    CHECK(c.Render(false, true) == "'\n");
    c.SetDot(0, 3, -1); // plus a bottom dot
    CHECK(c.Render(false, true) == ":\n");
  }
  {
    // Strong text (labels) wins over dots; weak text (axes) loses.
    Canvas c(2, 1);
    c.SetDot(0, 0, -1);
    c.SetText(0, 0, 'X', true);
    c.SetDot(2, 0, -1);
    c.SetText(1, 0, '|', false);
    std::string r = c.Render(false, false);
    CHECK(r[0] == 'X');
    CHECK(r.substr(1, 3) == "\xE2\xA0\x81");
  }
  {
    // Out-of-bounds writes are ignored rather than crashing.
    Canvas c(2, 2);
    c.SetDot(-1, 0, -1);
    c.SetDot(100, 100, -1);
    c.SetText(-5, 0, 'x', true);
    c.SetText(0, 99, 'x', true);
    CHECK(c.Render(false, false) == "\n\n");
  }
}

void TestRenderPlot() {
  PlotOptions opt;
  opt.width = 60;
  opt.height = 20;
  opt.color = false;

  // A sine plot contains curve cells and pi-based labels.
  std::string plot = RenderPlot({FindByKey('s')}, opt);
  CHECK(plot.find("\xCF\x80") != std::string::npos);  // pi label
  CHECK(plot.find("sine") != std::string::npos);      // legend
  CHECK(plot.find("\xE2") != std::string::npos);      // some Braille cell

  // ASCII mode emits no multi-byte characters at all.
  opt.ascii = true;
  std::string asciiPlot = RenderPlot({FindByKey('t')}, opt);
  for (char ch : asciiPlot)
    CHECK(static_cast<unsigned char>(ch) < 0x80);
  CHECK(asciiPlot.find("pi") != std::string::npos);
  CHECK(asciiPlot.find("tangent") != std::string::npos);

  // Color mode wraps curves in ANSI escapes.
  opt.ascii = false;
  opt.color = true;
  std::string colorPlot = RenderPlot({FindByKey('c')}, opt);
  CHECK(colorPlot.find("\x1b[95m") != std::string::npos);
  CHECK(colorPlot.find("\x1b[0m") != std::string::npos);
}

} // namespace

int main() {
  TestParseAngle();
  TestFormatPiFraction();
  TestNiceStep();
  TestLookup();
  TestEvaluate();
  TestCanvas();
  TestRenderPlot();

  if (failures == 0) {
    std::cout << "All tests passed.\n";
    return 0;
  }
  std::cerr << failures << " check(s) failed.\n";
  return 1;
}
