// Trigonometric Function Visualizer
// Plots sine, cosine, tangent, cosecant, secant, and cotangent in the
// terminal using Braille sub-cell resolution (with an ASCII fallback),
// ANSI colors, pi-based axis labels, overlays, and A*f(Bx + C) + D
// transforms. Run with function names for one-shot output, or with no
// arguments for an interactive menu.

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "plotter.hpp"

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace {

using namespace trigviz;

constexpr const char *kVersion = "2.0.0";

bool StdoutIsTerminal() {
#ifdef _WIN32
  return _isatty(_fileno(stdout)) != 0;
#else
  return isatty(fileno(stdout)) != 0;
#endif
}

void SetUpTerminal() {
#ifdef _WIN32
  SetConsoleOutputCP(CP_UTF8);
  HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD mode = 0;
  if (handle != INVALID_HANDLE_VALUE && GetConsoleMode(handle, &mode))
    SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
}

void PrintUsage() {
  std::cout <<
      R"(Trigonometric Function Visualizer )" << kVersion << R"(

Usage:
  trig_visualizer [options] [function ...]

With one or more functions on the command line the plot is printed once and
the program exits; with none, an interactive menu starts.

Functions (name or key; several may be combined into one plot):
  s  sin  sine        c  cos  cosine      t  tan  tangent
  C  csc  cosecant    S  sec  secant      T  cot  cotangent

Options (angles accept pi expressions: pi, -pi/2, 3pi/4, 2pi, 1.57):
  --xmin=V, --xmax=V     x range               (default -2pi .. 2pi)
  --ymax=V               y range is [-V, V]    (default: auto)
  --width=N, --height=N  plot size in characters (default 90 x 28)
  -a, --amplitude=V      plot V * f(x)         (default 1)
  -f, --frequency=V      plot f(V * x)         (default 1)
  -p, --phase=V          plot f(x + V)         (default 0)
  -o, --offset=V         plot f(x) + V         (default 0)
  --ascii                plain ASCII output instead of Braille dots
  --no-color             disable ANSI colors (also auto-off when piped)
  -h, --help             show this help
  --version              show version

Examples:
  trig_visualizer sin cos                 overlay sine and cosine
  trig_visualizer tan --ymax=6            tangent with a taller y range
  trig_visualizer sin -a 2 -f 2           plot 2*sin(2x)
  trig_visualizer sin --xmin=-pi --xmax=pi
)";
}

void PrintMenu() {
  std::cout << "\nTrigonometric Function Visualizer\n"
               "---------------------------------\n";
  for (const auto &f : kFunctions)
    std::cout << "  " << f.key << " = " << f.name << " (" << f.ratio << ")\n";
  std::cout << "\nEnter one or more functions to plot together\n"
               "(e.g. \"s\", \"sc\", \"sin cos\"), a = all, h = help, q = quit\n\n"
               "> " << std::flush;
}

// Resolves one token ("sin", "s", or a run of keys like "sc") into functions.
// Returns false if the token isn't recognized.
bool ResolveToken(const std::string &token,
                  std::vector<const TrigFunction *> &out) {
  if (const TrigFunction *f = FindByName(token)) {
    out.push_back(f);
    return true;
  }
  if (token == "a" || token == "all") {
    for (const auto &f : kFunctions)
      out.push_back(&f);
    return true;
  }
  std::vector<const TrigFunction *> keys;
  for (char ch : token) {
    const TrigFunction *f = FindByKey(ch);
    if (!f)
      return false;
    keys.push_back(f);
  }
  out.insert(out.end(), keys.begin(), keys.end());
  return true;
}

void Deduplicate(std::vector<const TrigFunction *> &funcs) {
  std::vector<const TrigFunction *> seen;
  for (const auto *f : funcs)
    if (std::find(seen.begin(), seen.end(), f) == seen.end())
      seen.push_back(f);
  funcs = seen;
}

std::string TitleFor(const std::vector<const TrigFunction *> &funcs,
                     const PlotOptions &opt) {
  std::string names;
  for (const auto *f : funcs) {
    if (!names.empty())
      names += ", ";
    names += f->name;
  }
  const Transform &t = opt.transform;
  std::string extra;
  if (t.amplitude != 1.0 || t.frequency != 1.0 || t.phase != 0.0 ||
      t.offset != 0.0) {
    std::ostringstream os;
    os << "  [y = ";
    if (t.amplitude != 1.0)
      os << FormatNumber(t.amplitude) << "*";
    os << "f(";
    if (t.frequency != 1.0)
      os << FormatNumber(t.frequency);
    os << "x";
    if (t.phase != 0.0)
      os << (t.phase > 0 ? " + " : " - ") << FormatNumber(std::fabs(t.phase));
    os << ")";
    if (t.offset != 0.0)
      os << (t.offset > 0 ? " + " : " - ") << FormatNumber(std::fabs(t.offset));
    os << "]";
    extra = os.str();
  }
  return names + extra;
}

void ShowPlot(std::vector<const TrigFunction *> funcs, const PlotOptions &opt) {
  Deduplicate(funcs);
  std::cout << '\n' << TitleFor(funcs, opt) << "\n\n"
            << RenderPlot(funcs, opt);
}

void RunInteractive(const PlotOptions &opt) {
  PrintMenu();
  std::string line;
  while (std::getline(std::cin, line)) {
    std::istringstream in(line);
    std::vector<const TrigFunction *> funcs;
    std::string token;
    bool quit = false, ok = true;
    while (in >> token) {
      if (token == "q" || token == "quit" || token == "exit") {
        quit = true;
        break;
      }
      if (token == "h" || token == "help") {
        PrintUsage();
        funcs.clear();
        break;
      }
      if (!ResolveToken(token, funcs)) {
        std::cout << "Unrecognized option '" << token << "'. Try again.\n";
        ok = false;
        break;
      }
    }
    if (quit)
      break;
    if (ok && !funcs.empty())
      ShowPlot(funcs, opt);
    PrintMenu();
  }
  std::cout << "Goodbye!\n";
}

struct ArgError {
  std::string message;
};

// Grabs the value for --flag=value / --flag value style options.
std::string OptionValue(const std::string &arg, int argc, char **argv, int &i) {
  size_t eq = arg.find('=');
  if (eq != std::string::npos)
    return arg.substr(eq + 1);
  if (i + 1 >= argc)
    throw ArgError{"missing value for " + arg};
  return argv[++i];
}

double NumberValue(const std::string &arg, int argc, char **argv, int &i) {
  std::string raw = OptionValue(arg, argc, argv, i);
  auto v = ParseAngle(raw);
  if (!v)
    throw ArgError{"invalid number '" + raw + "' for " + arg};
  return *v;
}

bool FlagMatches(const std::string &arg, const char *shortName,
                 const char *longName) {
  if (shortName && arg == shortName)
    return true;
  if (arg == longName)
    return true;
  return arg.rfind(std::string(longName) + "=", 0) == 0;
}

} // namespace

int main(int argc, char **argv) {
  SetUpTerminal();

  PlotOptions opt;
  opt.color = StdoutIsTerminal();
  std::vector<const TrigFunction *> funcs;

  try {
    for (int i = 1; i < argc; ++i) {
      std::string arg = argv[i];
      if (arg == "-h" || arg == "--help") {
        PrintUsage();
        return 0;
      } else if (arg == "--version") {
        std::cout << "trig_visualizer " << kVersion << '\n';
        return 0;
      } else if (arg == "--ascii") {
        opt.ascii = true;
      } else if (arg == "--no-color") {
        opt.color = false;
      } else if (arg == "--color") {
        opt.color = true;
      } else if (FlagMatches(arg, nullptr, "--width")) {
        opt.width = static_cast<int>(NumberValue(arg, argc, argv, i));
      } else if (FlagMatches(arg, nullptr, "--height")) {
        opt.height = static_cast<int>(NumberValue(arg, argc, argv, i));
      } else if (FlagMatches(arg, nullptr, "--xmin")) {
        opt.xmin = NumberValue(arg, argc, argv, i);
      } else if (FlagMatches(arg, nullptr, "--xmax")) {
        opt.xmax = NumberValue(arg, argc, argv, i);
      } else if (FlagMatches(arg, nullptr, "--ymax")) {
        opt.ymax = NumberValue(arg, argc, argv, i);
      } else if (FlagMatches(arg, "-a", "--amplitude")) {
        opt.transform.amplitude = NumberValue(arg, argc, argv, i);
      } else if (FlagMatches(arg, "-f", "--frequency")) {
        opt.transform.frequency = NumberValue(arg, argc, argv, i);
      } else if (FlagMatches(arg, "-p", "--phase")) {
        opt.transform.phase = NumberValue(arg, argc, argv, i);
      } else if (FlagMatches(arg, "-o", "--offset")) {
        opt.transform.offset = NumberValue(arg, argc, argv, i);
      } else if (!arg.empty() && arg[0] == '-' && arg.size() > 1 &&
                 !std::isdigit(static_cast<unsigned char>(arg[1]))) {
        throw ArgError{"unknown option " + arg + " (see --help)"};
      } else if (!ResolveToken(arg, funcs)) {
        throw ArgError{"unknown function '" + arg + "' (see --help)"};
      }
    }

    opt.width = std::clamp(opt.width, 40, 400);
    opt.height = std::clamp(opt.height, 10, 200);
    if (opt.xmax <= opt.xmin)
      throw ArgError{"--xmax must be greater than --xmin"};
    if (opt.transform.amplitude == 0.0)
      throw ArgError{"--amplitude must be nonzero"};
  } catch (const ArgError &e) {
    std::cerr << "error: " << e.message << '\n';
    return 1;
  }

  if (!funcs.empty()) {
    ShowPlot(funcs, opt);
    return 0;
  }

  RunInteractive(opt);
  return 0;
}
