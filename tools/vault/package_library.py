#!/usr/bin/env python3
"""CHROME VAULT sample library packaging helper (docs/CHROME_VAULT_INSTALL.md, "For developers").

The library (Library/library.json + Library/audio/**.ogg, built by VaultLibBuild) never lives on the main branch.
It is kept on the orphan branch "vault-library" of the same repository, laid out as
    Library/library.json
    Library/audio/<instrument>/<file>.ogg
    README.md
CI checks that branch out into ./vault-library, runs VaultTests on it and packages it into the installers.

Commands (standard library only, Python 3.8+; run from anywhere):
  find    <dir>                         print the library folder inside <dir> (<dir>/Library or <dir> itself) - exit 1 if none
  check   <libdir> [--max-file-mb 50]   validate library.json, every referenced file exists, every file < 50 MB
                                        (GitHub refuses files > 100 MB and warns > 50 MB); prints the packaged size
  size    <libdir>                      print the packaged byte count (library.json + referenced audio) only
  zip     <libdir> <outdir> [--name CHROME-VAULT-Library] [--max-part-bytes 1900000000]
                                        writes <name>.zip (root folder "Library/") or, when the library is larger than
                                        one part, <name>-part1-of-N.zip ... - every part is a complete zip of its own
                                        (no split archive), extracting all of them into one folder gives the library;
                                        prints the written file names, one per line
  branch  <libdir> [--repo .] [--branch vault-library] [--batch-mb 400]
                                        builds the orphan branch with git plumbing (the working tree and the current
                                        branch are not touched): one commit per ~400 MB batch, so each push stays far
                                        below GitHub's 2 GB push limit; prints the push commands. Never pushes.
Only library.json and the audio files it references are packaged (build reports and stale files are left out).
"""
import argparse
import json
import os
import subprocess
import sys
import tempfile
import zipfile

GITHUB_ASSET_LIMIT = 2_000_000_000   # stay below GitHub's 2 GiB release-asset limit with margin


def find_library(d):
    for cand in (os.path.join(d, "Library"), d):
        if os.path.isfile(os.path.join(cand, "library.json")):
            return os.path.abspath(cand)
    return None


def load(libdir):
    with open(os.path.join(libdir, "library.json"), "r", encoding="utf-8") as f:
        return json.load(f)


def referenced_files(lib):
    files = []
    seen = set()
    for inst in lib.get("instruments", []):
        for art in inst.get("articulations", []):
            for z in art.get("zones", []):
                rel = z.get("file", "")
                if rel and rel not in seen:
                    seen.add(rel)
                    files.append(rel)
    return files


def package_list(libdir):
    """(archive path below Library/, absolute source path) for library.json + every referenced audio file"""
    lib = load(libdir)
    out = [("library.json", os.path.join(libdir, "library.json"))]
    for rel in referenced_files(lib):
        out.append(("audio/" + rel.replace("\\", "/"), os.path.join(libdir, "audio", *rel.replace("\\", "/").split("/"))))
    return out


def cmd_find(a):
    p = find_library(a.dir)
    if not p:
        return 1
    print(p)
    return 0


def cmd_check(a):
    libdir = os.path.abspath(a.libdir)
    errors, warnings = [], []
    try:
        lib = load(libdir)
    except Exception as e:  # noqa: BLE001 - report any parse / IO problem
        print("ERROR: cannot read %s: %s" % (os.path.join(libdir, "library.json"), e))
        return 1
    if lib.get("version") != 1:
        errors.append("library.json version %r (expected 1)" % lib.get("version"))
    insts = lib.get("instruments", [])
    if not insts:
        errors.append("no instruments")
    ids = [i.get("id", "") for i in insts]
    if len(set(ids)) != len(ids):
        errors.append("duplicate instrument ids")
    limit = int(a.max_file_mb * 1024 * 1024)
    total, count = 0, 0
    for arc, src in package_list(libdir):
        if not os.path.isfile(src):
            errors.append("missing file: " + arc)
            continue
        sz = os.path.getsize(src)
        total += sz
        count += 1
        if sz >= limit:
            errors.append("%s is %.1f MB (limit %.0f MB per file)" % (arc, sz / 1048576, a.max_file_mb))
    refs = set(r.replace("\\", "/") for r in referenced_files(lib))
    stale = 0
    audio = os.path.join(libdir, "audio")
    for root, _, names in os.walk(audio):
        for n in names:
            rel = os.path.relpath(os.path.join(root, n), audio).replace(os.sep, "/")
            if rel not in refs:
                stale += 1
    if stale:
        warnings.append("%d file(s) in audio/ are not referenced by library.json (not packaged)" % stale)
    for w in warnings:
        print("warning: " + w)
    for e in errors[:50]:
        print("ERROR: " + e)
    print("library %s: %d instruments, %d files, %.1f MB packaged" % (libdir, len(insts), count, total / 1048576))
    if total >= GITHUB_ASSET_LIMIT:
        print("note: larger than one release asset - the zip is split into parts; the installers will not fit under 2 GB")
    return 1 if errors else 0


def cmd_size(a):
    print(sum(os.path.getsize(src) for _, src in package_list(os.path.abspath(a.libdir)) if os.path.isfile(src)))
    return 0


def cmd_zip(a):
    libdir = os.path.abspath(a.libdir)
    os.makedirs(a.outdir, exist_ok=True)
    items = package_list(libdir)
    missing = [arc for arc, src in items if not os.path.isfile(src)]
    if missing:
        print("ERROR: missing files, run check first: " + ", ".join(missing[:10]), file=sys.stderr)
        return 1
    # zip overhead per entry (local + central header, name twice) kept as margin
    cost = lambda arc, src: os.path.getsize(src) + 2 * len(arc) + 200  # noqa: E731
    parts, cur, cur_size = [], [], 0
    for arc, src in items:   # library.json first, then audio in library order (an instrument stays together where possible)
        c = cost(arc, src)
        if cur and cur_size + c > a.max_part_bytes:
            parts.append(cur)
            cur, cur_size = [], 0
        cur.append((arc, src))
        cur_size += c
    if cur:
        parts.append(cur)
    names = [a.name + ".zip"] if len(parts) == 1 else ["%s-part%d-of-%d.zip" % (a.name, i + 1, len(parts)) for i in range(len(parts))]
    for name, part in zip(names, parts):
        path = os.path.join(a.outdir, name)
        with zipfile.ZipFile(path, "w", allowZip64=True) as z:
            for arc, src in part:
                # OggVorbis does not compress: store it (fast, no size gain lost); deflate the json
                comp = zipfile.ZIP_DEFLATED if arc.endswith(".json") else zipfile.ZIP_STORED
                z.write(src, "Library/" + arc, compress_type=comp)
        if os.path.getsize(path) >= GITHUB_ASSET_LIMIT:
            print("ERROR: %s is %d bytes (GitHub limit)" % (name, os.path.getsize(path)), file=sys.stderr)
            return 1
        print(name)
    return 0


def git(repo, *args, env=None, inp=None):
    r = subprocess.run(["git", "-C", repo] + list(args), input=inp, capture_output=True, env=env, check=False)
    if r.returncode != 0:
        raise SystemExit("git %s failed: %s" % (" ".join(args[:2]), r.stderr.decode(errors="replace")))
    return r.stdout.decode().strip()


README = """# CHROME VAULT sample library (orphan branch)

This branch holds only the CHROME VAULT sample library: `Library/library.json` + `Library/audio/**.ogg`, built by
`VaultLibBuild` (main branch: `tools/vault/LibBuild.cpp`, `tools/vault/library_map.json`) from the CC0 1.0 recordings
of VSCO 2 Community Edition and the Versilian Community Sample Library (credits: `docs/CHROME_VAULT_CREDITS.md`).
It shares no history with the code. CI (`.github/workflows/build.yml`) checks it out into `./vault-library`, runs
`VaultTests` on it and packs it into the installers. Regenerate with `tools/vault/package_library.py branch`.
"""


def cmd_branch(a):
    repo = os.path.abspath(a.repo)
    libdir = os.path.abspath(a.libdir)
    items = package_list(libdir)
    missing = [arc for arc, src in items if not os.path.isfile(src)]
    if missing:
        raise SystemExit("missing files, run check first: " + ", ".join(missing[:10]))
    batch_bytes = int(a.batch_mb * 1024 * 1024)
    with tempfile.TemporaryDirectory() as tmp:
        env = dict(os.environ, GIT_INDEX_FILE=os.path.join(tmp, "index"))
        # README first, then the library in batches; library.json goes last so a partial branch is never "complete"
        readme = os.path.join(tmp, "README.md")
        with open(readme, "w", encoding="utf-8", newline="\n") as f:
            f.write(README)
        audio = [it for it in items if it[0] != "library.json"]
        batches, cur, size = [], [], 0
        for arc, src in audio:
            sz = os.path.getsize(src)
            if cur and size + sz > batch_bytes:
                batches.append(cur)
                cur, size = [], 0
            cur.append((arc, src))
            size += sz
        batches.append(cur + [("library.json", os.path.join(libdir, "library.json"))])
        batches[0] = [("../README.md", readme)] + batches[0]
        parent, commits = None, []
        for n, batch in enumerate(batches, 1):
            paths = "\n".join(src for _, src in batch) + "\n"
            shas = git(repo, "hash-object", "-w", "--no-filters", "--stdin-paths", env=env, inp=paths.encode()).split("\n")
            info = "".join("100644 %s\t%s\n" % (sha, os.path.normpath(os.path.join("Library", arc)).replace(os.sep, "/"))
                           for (arc, _), sha in zip(batch, shas))
            git(repo, "update-index", "--add", "--index-info", env=env, inp=info.encode())
            tree = git(repo, "write-tree", env=env)
            msg = "CHROME VAULT sample library (%d/%d)\n\nCo-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>\n" % (n, len(batches))
            args = ["commit-tree", tree, "-m", msg] + (["-p", parent] if parent else [])
            parent = git(repo, *args, env=env)
            commits.append(parent)
        git(repo, "update-ref", "refs/heads/" + a.branch, parent)
    print("branch %s -> %s (%d commits). Push one commit at a time (each push about %g MB or less):" % (a.branch, parent, len(commits), a.batch_mb))
    for c in commits:
        print("  git push origin %s:refs/heads/%s" % (c, a.branch))
    return 0


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="cmd", required=True)
    s = sub.add_parser("find"); s.add_argument("dir"); s.set_defaults(fn=cmd_find)
    s = sub.add_parser("check"); s.add_argument("libdir"); s.add_argument("--max-file-mb", type=float, default=50.0); s.set_defaults(fn=cmd_check)
    s = sub.add_parser("size"); s.add_argument("libdir"); s.set_defaults(fn=cmd_size)
    s = sub.add_parser("zip"); s.add_argument("libdir"); s.add_argument("outdir")
    s.add_argument("--name", default="CHROME-VAULT-Library"); s.add_argument("--max-part-bytes", type=int, default=1_900_000_000); s.set_defaults(fn=cmd_zip)
    s = sub.add_parser("branch"); s.add_argument("libdir"); s.add_argument("--repo", default=".")
    s.add_argument("--branch", default="vault-library"); s.add_argument("--batch-mb", type=float, default=400.0); s.set_defaults(fn=cmd_branch)
    a = p.parse_args()
    return a.fn(a)


if __name__ == "__main__":
    sys.exit(main())
