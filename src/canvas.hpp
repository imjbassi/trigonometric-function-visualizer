// A character-cell canvas that mixes Braille "pixels" (2x4 dots per cell)
// with plain text overlays (axes and labels), with optional ANSI color.
#pragma once

#include <string>
#include <vector>

namespace trigviz {

class Canvas {
public:
  Canvas(int cellsWide, int cellsHigh)
      : width_(cellsWide), height_(cellsHigh),
        cells_(static_cast<size_t>(cellsWide) * cellsHigh) {}

  int Width() const { return width_; }
  int Height() const { return height_; }
  int DotWidth() const { return width_ * 2; }
  int DotHeight() const { return height_ * 4; }

  // Sets one Braille dot; (0,0) is the top-left dot.
  void SetDot(int dx, int dy, int color) {
    if (dx < 0 || dy < 0 || dx >= DotWidth() || dy >= DotHeight())
      return;
    static const unsigned char kBits[4][2] = {
        {0x01, 0x08}, {0x02, 0x10}, {0x04, 0x20}, {0x40, 0x80}};
    Cell &c = At(dx / 2, dy / 4);
    c.dots |= kBits[dy % 4][dx % 2];
    c.dotColor = color;
  }

  // Places one glyph of text (a single display column; may be a multi-byte
  // UTF-8 sequence). "Weak" text (axes) is drawn under curve dots; "strong"
  // text (labels) is drawn over them.
  void SetText(int col, int row, const std::string &glyph, bool strong,
               int color = -1) {
    if (col < 0 || row < 0 || col >= width_ || row >= height_)
      return;
    Cell &c = At(col, row);
    if (!c.text.empty() && c.strong && !strong)
      return; // never let an axis overwrite a label
    c.text = glyph;
    c.strong = strong;
    c.textColor = color;
  }
  void SetText(int col, int row, char ch, bool strong, int color = -1) {
    SetText(col, row, std::string(1, ch), strong, color);
  }

  // Splits a UTF-8 string into glyphs, one per cell.
  static std::vector<std::string> SplitGlyphs(const std::string &text) {
    std::vector<std::string> glyphs;
    for (char ch : text) {
      if ((static_cast<unsigned char>(ch) & 0xC0) == 0x80 && !glyphs.empty())
        glyphs.back() += ch; // UTF-8 continuation byte
      else
        glyphs.emplace_back(1, ch);
    }
    return glyphs;
  }

  void SetLabel(int col, int row, const std::string &text, int color = -1) {
    std::vector<std::string> glyphs = SplitGlyphs(text);
    for (size_t i = 0; i < glyphs.size(); ++i)
      SetText(col + static_cast<int>(i), row, glyphs[i], true, color);
  }

  // Renders the canvas. With ascii set, Braille cells degrade to ' . : chars.
  std::string Render(bool useColor, bool ascii) const {
    std::string out;
    for (int row = 0; row < height_; ++row) {
      int current = kNoColor;
      std::string line;
      for (int col = 0; col < width_; ++col) {
        const Cell &c = At(col, row);
        int color = -1;
        std::string glyph = " ";
        if (!c.text.empty() && (c.strong || c.dots == 0)) {
          glyph = c.text;
          color = c.textColor;
        } else if (c.dots != 0) {
          color = c.dotColor;
          if (ascii) {
            bool upper = (c.dots & 0x1B) != 0;
            bool lower = (c.dots & 0xE4) != 0;
            glyph = (upper && lower) ? ":" : (upper ? "'" : ".");
          } else {
            glyph.clear();
            AppendUtf8(glyph, 0x2800 + c.dots);
          }
        }
        if (useColor && color != current) {
          line += (color < 0) ? "\x1b[0m" : "\x1b[" + std::to_string(color) + "m";
          current = color;
        }
        line += glyph;
      }
      if (useColor && current != -1)
        line += "\x1b[0m";
      // Trim trailing spaces (safe: escapes never end a line with a space).
      size_t end = line.find_last_not_of(' ');
      out += (end == std::string::npos) ? "" : line.substr(0, end + 1);
      out += '\n';
    }
    return out;
  }

private:
  struct Cell {
    unsigned char dots = 0;
    std::string text;
    bool strong = false;
    int dotColor = -1;
    int textColor = -1;
  };

  static constexpr int kNoColor = -2;

  Cell &At(int col, int row) { return cells_[static_cast<size_t>(row) * width_ + col]; }
  const Cell &At(int col, int row) const {
    return cells_[static_cast<size_t>(row) * width_ + col];
  }

  static void AppendUtf8(std::string &out, unsigned cp) {
    out += static_cast<char>(0xE0 | (cp >> 12));
    out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
    out += static_cast<char>(0x80 | (cp & 0x3F));
  }

  int width_;
  int height_;
  std::vector<Cell> cells_;
};

} // namespace trigviz
