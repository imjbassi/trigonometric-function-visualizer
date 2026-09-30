"""Renders real trig_visualizer output as a terminal-style PNG screenshot.

Runs the built binary, captures its ANSI-colored output, and draws it onto
a dark terminal window so the README can show the tool exactly as it looks.

Usage:
    cmake -B build && cmake --build build
    pip install Pillow
    python assets/generate_screenshot.py [path/to/trig_visualizer]
"""

import re
import subprocess
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

FONT_PATH = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf"
# DejaVu Sans Mono has no Braille block; FreeMono does, at ~the same advance.
BRAILLE_FONT_PATH = "/usr/share/fonts/truetype/freefont/FreeMono.ttf"
FONT_SIZE = 26
BG = "#0d1117"
CHROME = "#161b22"
DEFAULT_FG = "#e6edf3"
PROMPT = "#7ee787"
COLORS = {
    90: "#8b949e",  # axes: gray
    91: "#ff7b72",  # secant: red
    92: "#56d364",  # cosecant: green
    93: "#e3b341",  # tangent: yellow
    94: "#79c0ff",  # cotangent: blue
    95: "#d2a8ff",  # cosine: magenta
    96: "#39c5cf",  # sine: cyan
}

ANSI_RE = re.compile(r"\x1b\[(\d+)m")


def parse_ansi(text):
    """Yields lines of (char, color) pairs from ANSI-colored text."""
    lines = []
    for raw in text.splitlines():
        cells = []
        color = None
        pos = 0
        for match in ANSI_RE.finditer(raw):
            for ch in raw[pos:match.start()]:
                cells.append((ch, color))
            code = int(match.group(1))
            color = None if code == 0 else COLORS.get(code)
            pos = match.end()
        for ch in raw[pos:]:
            cells.append((ch, color))
        lines.append(cells)
    return lines


def main():
    binary = sys.argv[1] if len(sys.argv) > 1 else "build/trig_visualizer"
    args = ["sin", "cos", "--color", "--width=78", "--height=22"]
    out = subprocess.run([binary] + args, capture_output=True, text=True,
                         check=True).stdout
    lines = parse_ansi(out)
    # Prepend the shell prompt line the screenshot pretends to show.
    prompt = [("$", PROMPT), (" ", None)]
    prompt += [(c, DEFAULT_FG) for c in "trig_visualizer sin cos"]
    lines = [prompt] + lines

    font = ImageFont.truetype(FONT_PATH, FONT_SIZE)
    braille_font = ImageFont.truetype(BRAILLE_FONT_PATH, FONT_SIZE)
    cell_w = font.getlength("M")
    ascent, descent = font.getmetrics()
    cell_h = int((ascent + descent) * 1.12)

    pad = 28
    bar_h = 56
    cols = max(len(line) for line in lines)
    width = int(cols * cell_w) + 2 * pad
    height = bar_h + len(lines) * cell_h + 2 * pad

    img = Image.new("RGB", (width, height), BG)
    draw = ImageDraw.Draw(img)

    # Title bar with traffic-light buttons.
    draw.rectangle([0, 0, width, bar_h], fill=CHROME)
    for i, dot in enumerate(("#ff5f56", "#ffbd2e", "#27c93f")):
        x = 26 + i * 38
        draw.ellipse([x, bar_h // 2 - 10, x + 20, bar_h // 2 + 10], fill=dot)
    title = "trig_visualizer"
    tw = draw.textlength(title, font=font)
    draw.text(((width - tw) / 2, (bar_h - ascent - descent) / 2), title,
              font=font, fill="#8b949e")

    for row, line in enumerate(lines):
        y = bar_h + pad + row * cell_h
        for col, (ch, color) in enumerate(line):
            if ch == " ":
                continue
            f = braille_font if 0x2800 <= ord(ch) <= 0x28FF else font
            draw.text((pad + col * cell_w, y), ch, font=f,
                      fill=color or DEFAULT_FG)

    out_path = Path(__file__).parent / "terminal_demo.png"
    img.save(out_path)
    print(f"wrote {out_path} ({width}x{height})")


if __name__ == "__main__":
    main()
