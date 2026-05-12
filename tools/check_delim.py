# -*- coding: utf-8 -*-
import re
from pathlib import Path
p = Path(__file__).resolve().parents[1] / "web_assets.h"
data = p.read_text(encoding="utf-8")
for name in ("DELIM_JS", "DELIM_CSS"):
    pat = ")" + name + '"'
    for m in re.finditer(re.escape(pat), data):
        line = data.count("\n", 0, m.start()) + 1
        print(name, "at line", line, "offset", m.start())
