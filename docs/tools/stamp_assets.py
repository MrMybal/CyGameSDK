"""Refresh stylesheet/script cache keys after editing the static site."""
from pathlib import Path
import hashlib
import re
root = Path(__file__).resolve().parents[1]
for page in (root / "index.html", root / "guide.html"):
    text = page.read_text("utf-8")
    for name in ("styles.css", "site-data.js", "site.js"):
        digest = hashlib.sha256((root / name).read_bytes()).hexdigest()[:12]
        text = re.sub(re.escape(name) + r"\?v=[^\"']+", name + "?v=" + digest, text)
    page.write_text(text, encoding="utf-8")
print("Styles and scripts use content-based cache keys.")
