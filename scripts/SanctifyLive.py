#!/usr/bin/env python3
"""
SanctifyLive.py

Zip this repository into SanctifyLive.zip at the repository root.
Usage:
  python scripts/SanctifyLive.py [--output PATH] [--force]

The script will skip unreadable/locked files and report skipped items.
"""
from pathlib import Path
import zipfile
import argparse
import sys
import os
from typing import Iterable
import fnmatch


def load_gitignore(root: Path):
    gitignore = root / '.gitignore'
    patterns = []
    if not gitignore.exists():
        return patterns
    try:
        for raw in gitignore.read_text(encoding='utf-8', errors='surrogateescape').splitlines():
            line = raw.strip()
            if not line or line.startswith('#'):
                continue
            patterns.append(line)
    except Exception:
        return []
    return patterns


def matches_gitignore(rel_path: str, patterns) -> bool:
    """Return True if rel_path (posix) matches gitignore patterns.

    This implements a small, order-aware subset of gitignore semantics:
    - supports negation starting with `!`
    - patterns ending with `/` match directories and their contents
    - patterns without a leading `/` are matched anywhere (we prepend `**/`)
    """
    if not patterns:
        return False
    matched = False
    for pat in patterns:
        neg = pat.startswith('!')
        raw = pat[1:] if neg else pat
        p = raw.replace('\\', '/').strip()
        if p == '' or p.startswith('#'):
            continue

        anchored = p.startswith('/')
        if anchored:
            p = p.lstrip('/')

        # directory-only pattern (ends with '/') should match directory and contents
        is_dir_pattern = p.endswith('/')
        if is_dir_pattern:
            p = p.rstrip('/')

        # normalize rel_path and pattern for prefix checks
        rel = rel_path

        # Direct prefix match: if pattern equals the path segment or is a parent dir
        if rel == p or rel.startswith(p + '/'):
            matched = not neg
            continue

        # Try glob matches. If anchored, don't prepend '**/'
        glob_base = p
        if not anchored:
            glob_base = '**/' + glob_base

        # patterns that should also match directory contents
        glob_candidates = [glob_base]
        if is_dir_pattern or ('/' not in raw and not any(ch in raw for ch in '*?[')):
            # for plain names like '.docs' match contents too
            glob_candidates.append(glob_base.rstrip('/') + '/**')
        else:
            # also try matching any nested contents for non-anchored patterns
            glob_candidates.append(glob_base + '/**')

        for gp in glob_candidates:
            try:
                if fnmatch.fnmatchcase(rel, gp):
                    matched = not neg
                    break
            except Exception:
                pass

    return matched


def parse_root_ignores(ignore_list: Iterable[str]) -> set:
    """Normalize an iterable of root-relative ignore paths into a set of posix paths.

    Examples accepted: 'docs', 'docs/', '/docs', 'path/to/file.txt'
    Returned paths never start or end with '/'.
    """
    out = set()
    for s in (ignore_list or []):
        if not s:
            continue
        p = s.replace('\\', '/').strip()
        p = p.lstrip('/')
        p = p.rstrip('/')
        if p:
            out.add(p)
    return out


def make_zip(root: Path, out: Path, force: bool = False, exclude_dirs=None, root_ignores=None) -> None:
    out = out.resolve()
    if out.exists():
        if force:
            try:
                out.unlink()
            except Exception as e:
                print(f"Failed to remove existing {out}: {e}", file=sys.stderr)
                sys.exit(1)
        else:
            print(f"{out} already exists. Use --force to overwrite.", file=sys.stderr)
            sys.exit(1)

    exclude_dirs = set(exclude_dirs or [])
    root_ignores = parse_root_ignores(root_ignores)

    gitignore_patterns = load_gitignore(root)

    skipped = 0
    added = 0

    with zipfile.ZipFile(out, "w", compression=zipfile.ZIP_DEFLATED) as zf:
        for p in sorted(root.rglob("*")):
            # Skip the output zip itself if it sits under the repo root
            try:
                if p.resolve() == out:
                    continue
            except Exception:
                pass

            try:
                rel = p.relative_to(root).as_posix()
            except Exception:
                rel = p.name


            # skip by simple exclude_dirs (top-level directory names)
            if any(rel == d or rel.startswith(d + '/') for d in exclude_dirs):
                continue

            # skip by explicit root-relative ignores (files or directories)
            try:
                if any(rel == r or rel.startswith(r + '/') for r in root_ignores):
                    continue
            except Exception:
                pass

            # respect .gitignore patterns
            try:
                if matches_gitignore(rel, gitignore_patterns):
                    # If a .gitignore negation later re-includes, matches_gitignore handles that
                    # skip this path
                    continue
            except Exception:
                pass

            if p.is_dir():
                # add empty dir entries
                try:
                    if not any(p.iterdir()):
                        zf.writestr(rel.rstrip('/') + '/', '')
                except Exception:
                    skipped += 1
                continue

            if p.is_file():
                try:
                    zf.write(p, rel)
                    added += 1
                except (PermissionError, FileNotFoundError, OSError) as e:
                    print(f"Skipping {p}: {e}", file=sys.stderr)
                    skipped += 1
                except Exception as e:
                    print(f"Unexpected error adding {p}: {e}", file=sys.stderr)
                    skipped += 1

    print(f"Created {out} — added {added} files, skipped {skipped} items")


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Zip repository into SanctifyLive.zip at repo root")
    p.add_argument("--output", "-o", type=Path, default=None, help="Output zip path (default: repo_root/SanctifyLive.zip)")
    p.add_argument("--force", "-f", action="store_true", default=True,
                   help="Overwrite existing zip if present (default: always overwrite)")
    p.add_argument("--exclude", "-e", default=".vs,.git,build,bin,__pycache__",
                   help="Comma-separated directory names to exclude (default: .vs,.git,build,bin,__pycache__)")
    p.add_argument("--ignore-root", "-i", default="docs/",
                   help="Comma-separated root-relative paths to ignore (default: docs/)")
    return p.parse_args()


def main() -> None:
    args = parse_args()
    script_path = Path(__file__).resolve()
    repo_root = script_path.parent.parent.resolve()

    out = args.output.resolve() if args.output else (repo_root / "SanctifyLive.zip").resolve()
    exclude_dirs = [d for d in (s.strip() for s in args.exclude.split(',')) if d]
    root_ignores = [d for d in (s.strip() for s in args.ignore_root.split(',')) if d]

    make_zip(repo_root, out, force=args.force, exclude_dirs=exclude_dirs, root_ignores=root_ignores)


if __name__ == '__main__':
    main()
