#!/usr/bin/env python3
"""Build the owner's CHROME VAULT listening page: one self-contained HTML file, audio embedded as base64 OGG.

usage: listen_page.py picks.tsv ogg_dir out.html
picks.tsv columns: id, name, category, layers (" + " separated), recipe
"""
import base64, html, json, os, sys

CZ = {
    "Piano": "Klavír", "Keys & Organ": "Klávesy a varhany", "Guitars": "Kytary", "Bass": "Basy",
    "Strings": "Smyčce", "Brass": "Žestě", "Flutes & Woodwinds": "Flétny a dřevěné dechy", "Orchestral": "Orchestr",
    "Bells & Mallets": "Zvony a paličkové nástroje", "Vocals & Choir": "Hlasy a sbor", "Plucks": "Drnkavé zvuky",
    "Leads": "Sólové syntezátory", "Pads": "Plochy (pady)", "Synth": "Syntezátory", "World": "Nástroje světa",
    "FX & Atmos": "Efekty a atmosféry",
}
ORDER = list(CZ.keys())


def main():
    picks_path, ogg_dir, out = sys.argv[1:4]
    rows = []
    for line in open(picks_path, encoding="utf-8"):
        c = line.rstrip("\n").split("\t")
        if len(c) < 4 or c[0] == "id":
            continue
        f = os.path.join(ogg_dir, c[0] + ".ogg")
        if not os.path.exists(f):
            print("missing", f)
            continue
        rows.append({"id": c[0], "name": c[1], "cat": c[2], "layers": c[3],
                     "b64": base64.b64encode(open(f, "rb").read()).decode("ascii")})
    rows.sort(key=lambda r: (ORDER.index(r["cat"]) if r["cat"] in ORDER else 99))

    sections = []
    for cat in ORDER:
        rs = [r for r in rows if r["cat"] == cat]
        if not rs:
            continue
        items = []
        for r in rs:
            items.append(f"""
      <li class="row" data-id="{html.escape(r['id'])}" data-name="{html.escape(r['name'])}" data-cat="{html.escape(cat)}">
        <div class="meta">
          <div class="name">{html.escape(r['name'])}</div>
          <div class="sub"><span class="chip">{html.escape(cat)}</span><span class="layers">{html.escape(r['layers'])}</span><span class="pid">{html.escape(r['id'])}</span></div>
        </div>
        <audio controls preload="none" src="data:audio/ogg;base64,{r['b64']}"></audio>
        <div class="vote">
          <button type="button" class="up" aria-pressed="false" title="I like it">&#128077;</button>
          <button type="button" class="down" aria-pressed="false" title="I don't like it">&#128078;</button>
        </div>
      </li>""")
        sections.append(f"""
    <section>
      <h2>{html.escape(CZ[cat])} <span class="en">{html.escape(cat)}</span></h2>
      <ul>{''.join(items)}
      </ul>
    </section>""")

    page = f"""<!doctype html>
<html lang="cs">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>CHROME VAULT Listening</title>
<style>
:root {{
  color-scheme: dark;
  --ink: #0b0f16; --panel: #111722; --panel2: #151c28; --line: #222b3a; --text: #e3e8f0; --muted: #a9b6cc;
  --dim: #8593ab; --ice: #9fc3ff; --ice2: #7fa6e8; --deep: #4676c4; --field: #05080d; --good: #8fd6a8; --bad: #f0a3a3;
}}
* {{ box-sizing: border-box; }}
body {{ margin: 0; background: var(--ink); color: var(--text); font: 15px/1.45 system-ui, -apple-system, "Segoe UI", sans-serif; }}
header {{ padding: 28px 16px 18px; background: linear-gradient(#121a28, #0a0f18); border-bottom: 1px solid var(--line); }}
.wrap {{ max-width: 980px; margin: 0 auto; padding: 0 16px; }}
header .wrap {{ padding: 0; }}
h1 {{ margin: 0; font-size: 30px; letter-spacing: .08em; font-weight: 800;
      background: linear-gradient(#ffffff, #e6eaf2 45%, #7d8ba3 52%, #c9d2e2); -webkit-background-clip: text; background-clip: text; color: transparent; }}
.by {{ color: var(--dim); font-size: 13px; letter-spacing: .12em; text-transform: uppercase; }}
.intro {{ color: var(--muted); max-width: 70ch; }}
h2 {{ font-size: 18px; margin: 30px 0 10px; padding-bottom: 6px; border-bottom: 1px solid var(--line); }}
h2 .en {{ color: var(--dim); font-weight: 500; font-size: 13px; margin-left: 8px; }}
h3 {{ margin: 0 0 8px; font-size: 16px; }}
ul {{ list-style: none; margin: 0; padding: 0; }}
.row {{ display: grid; grid-template-columns: minmax(0, 1fr) minmax(220px, 300px) auto; gap: 12px; align-items: center;
        padding: 10px 12px; border: 1px solid var(--line); border-radius: 10px; background: var(--panel); margin-bottom: 8px; }}
.row.liked {{ border-color: #3f7a57; }}
.row.disliked {{ border-color: #7a3f3f; }}
.name {{ font-weight: 650; font-size: 16px; }}
.sub {{ display: flex; flex-wrap: wrap; gap: 8px; align-items: center; color: var(--muted); font-size: 13px; margin-top: 2px; }}
.chip {{ border: 1px solid #2a3548; border-radius: 99px; padding: 0 8px; color: var(--ice); font-size: 12px; }}
.pid {{ color: var(--dim); font-family: ui-monospace, monospace; font-size: 12px; }}
audio {{ width: 100%; height: 36px; }}
.vote {{ display: flex; gap: 6px; }}
.vote button {{ font-size: 20px; width: 46px; height: 40px; border-radius: 9px; border: 1px solid #2a3548; background: #0b111c; cursor: pointer; color: var(--text); }}
.vote button:hover {{ border-color: var(--ice2); }}
.vote button:focus-visible {{ outline: 2px solid var(--ice); outline-offset: 2px; }}
.vote .up[aria-pressed="true"] {{ background: #173323; border-color: var(--good); }}
.vote .down[aria-pressed="true"] {{ background: #3a1919; border-color: var(--bad); }}
.summary {{ margin: 34px 0 60px; padding: 16px; border: 1px solid var(--line); border-radius: 12px; background: var(--panel2); }}
.counts {{ color: var(--muted); margin-bottom: 8px; }}
textarea {{ width: 100%; min-height: 180px; background: var(--field); color: var(--text); border: 1px solid #243049; border-radius: 8px;
            padding: 10px; font: 13px/1.4 ui-monospace, monospace; }}
.actions {{ display: flex; gap: 8px; margin-top: 8px; flex-wrap: wrap; }}
.actions button {{ padding: 8px 14px; border-radius: 8px; border: 1px solid #2a3548; background: #0b111c; color: var(--text); cursor: pointer; font-size: 14px; }}
.actions button.primary {{ background: var(--deep); border-color: var(--ice2); }}
.note {{ color: var(--dim); font-size: 13px; }}
@media (max-width: 680px) {{
  .row {{ grid-template-columns: 1fr; }}
  .vote {{ justify-content: flex-end; }}
}}
</style>
</head>
<body>
<header><div class="wrap">
  <div class="by">TrapWin</div>
  <h1>CHROME VAULT</h1>
  <h3>Poslechová zkouška v2</h3>
  <p class="intro">{len(rows)} presets, 4 per category. Each clip plays the home phrase at C5 (FL Studio naming, MIDI 60):
  C5 E5 G5 C6, then a C5 major chord, 4 seconds. Press &#128077; or &#128078; on each row. Your choices are saved in this browser;
  the summary at the bottom can be copied and sent back.</p>
</div></header>
<main class="wrap">
{''.join(sections)}
  <section class="summary" aria-live="polite">
    <h2>Shrnutí <span class="en">Summary</span></h2>
    <div class="counts" id="counts"></div>
    <textarea id="summary" readonly aria-label="Summary text"></textarea>
    <div class="actions">
      <button type="button" class="primary" id="copy">Copy summary</button>
      <button type="button" id="reset">Clear all choices</button>
    </div>
    <p class="note">Copy the text above and paste it into the chat.</p>
  </section>
</main>
<script>
(function () {{
  var KEY = "chrome-vault-listen-v2";
  var votes = {{}};
  try {{ votes = JSON.parse(localStorage.getItem(KEY) || "{{}}") || {{}}; }} catch (e) {{ votes = {{}}; }}
  function save() {{ try {{ localStorage.setItem(KEY, JSON.stringify(votes)); }} catch (e) {{}} }}
  var rows = Array.prototype.slice.call(document.querySelectorAll(".row"));
  function paint() {{
    var up = [], down = [], open = 0;
    rows.forEach(function (r) {{
      var v = votes[r.dataset.id];
      r.classList.toggle("liked", v === "up");
      r.classList.toggle("disliked", v === "down");
      r.querySelector(".up").setAttribute("aria-pressed", v === "up" ? "true" : "false");
      r.querySelector(".down").setAttribute("aria-pressed", v === "down" ? "true" : "false");
      var line = r.dataset.id + "  " + r.dataset.name + "  (" + r.dataset.cat + ")";
      if (v === "up") up.push(line); else if (v === "down") down.push(line); else open++;
    }});
    document.getElementById("counts").textContent = up.length + " liked, " + down.length + " disliked, " + open + " not rated yet";
    var t = "CHROME VAULT v2 listening test\\n\\nLIKED (" + up.length + "):\\n" + (up.join("\\n") || "-") +
            "\\n\\nDISLIKED (" + down.length + "):\\n" + (down.join("\\n") || "-") + "\\n\\nNot rated: " + open + "\\n";
    document.getElementById("summary").value = t;
  }}
  rows.forEach(function (r) {{
    ["up", "down"].forEach(function (k) {{
      r.querySelector("." + k).addEventListener("click", function () {{
        votes[r.dataset.id] = votes[r.dataset.id] === k ? undefined : k;
        if (!votes[r.dataset.id]) delete votes[r.dataset.id];
        save(); paint();
      }});
    }});
  }});
  document.querySelectorAll("audio").forEach(function (a) {{
    a.addEventListener("play", function () {{
      document.querySelectorAll("audio").forEach(function (b) {{ if (b !== a) b.pause(); }});
    }});
  }});
  document.getElementById("copy").addEventListener("click", function () {{
    var ta = document.getElementById("summary");
    var done = function () {{ document.getElementById("copy").textContent = "Copied"; setTimeout(function () {{ document.getElementById("copy").textContent = "Copy summary"; }}, 1500); }};
    if (navigator.clipboard && navigator.clipboard.writeText) navigator.clipboard.writeText(ta.value).then(done, function () {{ ta.select(); document.execCommand("copy"); done(); }});
    else {{ ta.select(); document.execCommand("copy"); done(); }}
  }});
  document.getElementById("reset").addEventListener("click", function () {{
    if (confirm("Clear all choices?")) {{ votes = {{}}; save(); paint(); }}
  }});
  paint();
}})();
</script>
</body>
</html>
"""
    os.makedirs(os.path.dirname(out), exist_ok=True)
    open(out, "w", encoding="utf-8").write(page)
    print("wrote", out, len(rows), "rows", round(os.path.getsize(out) / 1e6, 2), "MB")


if __name__ == "__main__":
    main()
