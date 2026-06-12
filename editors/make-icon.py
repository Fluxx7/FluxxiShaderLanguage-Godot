"""Regenerate the FSL file icon (text rendered as SVG paths, no font dependency).

Usage:
    python3 make-icon.py [--text FSL] [--color "#b472ea"] [--font /path/to/font.ttf]

Requires fontTools (pip install fonttools). The default font is Lato Bold; if the
TTF isn't present it is downloaded from the Google Fonts repo (OFL licensed):
    https://github.com/google/fonts/raw/main/ofl/lato/Lato-Bold.ttf

Output is written to fsl-vscode/icons/fsl.svg and fsl-rider/src/main/resources/icons/fsl.svg.
"""
import argparse
import os
import urllib.request

from fontTools.ttLib import TTFont
from fontTools.pens.svgPathPen import SVGPathPen
from fontTools.pens.boundsPen import BoundsPen

HERE = os.path.dirname(os.path.abspath(__file__))
DEFAULT_FONT = "/tmp/Lato-Bold.ttf"
FONT_URL = "https://github.com/google/fonts/raw/main/ofl/lato/Lato-Bold.ttf"
OUTPUTS = [
    os.path.join(HERE, "fsl-vscode/icons/fsl.svg"),
    os.path.join(HERE, "fsl-rider/src/main/resources/icons/fsl.svg"),
]
VIEW = 16
MARGIN = 0.5


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--text", default="FSL")
    ap.add_argument("--color", default="#b472ea")
    ap.add_argument("--font", default=DEFAULT_FONT)
    args = ap.parse_args()

    if args.font == DEFAULT_FONT and not os.path.exists(args.font):
        print(f"downloading {FONT_URL}")
        urllib.request.urlretrieve(FONT_URL, args.font)

    font = TTFont(args.font)
    glyph_set = font.getGlyphSet()
    cmap = font.getBestCmap()

    # Outline each glyph and track tight bounds of the assembled text (font units).
    parts = []
    minx, miny, maxx, maxy = 1e9, 1e9, -1e9, -1e9
    x = 0
    for ch in args.text:
        glyph = glyph_set[cmap[ord(ch)]]
        pen = SVGPathPen(glyph_set)
        glyph.draw(pen)
        if pen.getCommands():
            parts.append((x, pen.getCommands()))
        bp = BoundsPen(glyph_set)
        glyph.draw(bp)
        if bp.bounds:
            gx0, gy0, gx1, gy1 = bp.bounds
            minx = min(minx, gx0 + x)
            maxx = max(maxx, gx1 + x)
            miny = min(miny, gy0)
            maxy = max(maxy, gy1)
        x += glyph.width

    w, h = maxx - minx, maxy - miny
    scale = (VIEW - 2 * MARGIN) / w
    tx = MARGIN - minx * scale
    ty = VIEW / 2 + (h / 2 + miny) * scale  # font Y-up -> SVG Y-down

    paths = "".join(
        f'<path transform="translate({gx},0)" d="{d}"/>' for gx, d in parts
    )
    svg = (
        f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {VIEW} {VIEW}">'
        f'<g fill="{args.color}" transform="translate({tx:.4f},{ty:.4f}) '
        f'scale({scale:.6f},{-scale:.6f})">{paths}</g></svg>'
    )

    for out in OUTPUTS:
        os.makedirs(os.path.dirname(out), exist_ok=True)
        with open(out, "w") as f:
            f.write(svg)
        print(f"wrote {out}")


if __name__ == "__main__":
    main()
