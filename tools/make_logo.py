#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""DARK-1's logo for the web pages: a ritual circle, a pentagram and an eye (drawn here, nothing borrowed), as PNGs:
web/img/dark1.png (92 x 92) and web/img/favicon.png (32 x 32).  python tools/make_logo.py [--inline]
--inline rewrites the logo and the favicon (data: URIs) in web/amb_installer.html, web/editor/index.html and web/emu/index.html."""
import base64
import io
import math
import os
import re
import sys

from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))
BG, RING, STAR, EYE, IRIS, PUPIL = (20, 14, 12), (156, 29, 58), (224, 52, 79), (244, 232, 200), (111, 224, 210), (20, 14, 12)


def draw(size):
    k = 4                                              # drawn four times larger, then reduced (smooth edges)
    n = size * k
    im = Image.new("RGBA", (n, n), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    c, r = n / 2, n * 0.46
    d.ellipse([c - r, c - r, c + r, c + r], fill=BG + (255,), outline=RING + (255,), width=max(2, n // 40))
    pts = [(c + r * 0.86 * math.cos(-math.pi / 2 + i * 2 * math.pi / 5), c + r * 0.86 * math.sin(-math.pi / 2 + i * 2 * math.pi / 5)) for i in range(5)]
    for i in range(5):
        d.line([pts[i], pts[(i + 2) % 5]], fill=STAR + (255,), width=max(2, n // 48))
    w, h = r * 0.62, r * 0.34                          # the eye: an almond
    top = [(c + x, c - h * (1 - (x / w) ** 2)) for x in [w * (j / 20 - 1) for j in range(41)]]
    bot = [(x, 2 * c - y) for x, y in reversed(top)]
    d.polygon(top + bot, fill=EYE + (255,))
    ir = h * 0.95
    d.ellipse([c - ir, c - ir, c + ir, c + ir], fill=IRIS + (255,))
    pw = max(2, n // 60)
    d.rectangle([c - pw, c - ir, c + pw, c + ir], fill=PUPIL + (255,))
    return im.resize((size, size), Image.LANCZOS)


def png(im):
    b = io.BytesIO()
    im.save(b, "PNG", optimize=True)
    return b.getvalue()


def main():
    logo, fav = png(draw(92)), png(draw(32))
    os.makedirs(os.path.join(HERE, "..", "web", "img"), exist_ok=True)
    open(os.path.join(HERE, "..", "web", "img", "dark1.png"), "wb").write(logo)
    open(os.path.join(HERE, "..", "web", "img", "favicon.png"), "wb").write(fav)
    print("dark1.png %d B, favicon.png %d B" % (len(logo), len(fav)))
    if "--inline" in sys.argv:
        d1 = "data:image/png;base64," + base64.b64encode(logo).decode()
        d2 = "data:image/png;base64," + base64.b64encode(fav).decode()
        for name in ("amb_installer.html", os.path.join("editor", "index.html"), os.path.join("emu", "index.html")):
            p = os.path.join(HERE, "..", "web", name)
            t = open(p, encoding="utf-8").read()
            t = re.sub(r'(<img class="logo" src=")[^"]*(")', lambda m: m.group(1) + d1 + m.group(2), t)
            t = re.sub(r'(<link rel="icon" type="image/png" href=")[^"]*(")', lambda m: m.group(1) + d2 + m.group(2), t)
            open(p, "w", encoding="utf-8", newline="\n").write(t)
            print("inlined into", p)


if __name__ == "__main__":
    main()
