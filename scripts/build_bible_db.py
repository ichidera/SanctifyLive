#!/usr/bin/env python3
"""
Builds/updates resources/scripture/bibles.sqlite from scrollmapper's
bible_databases JSON format (https://github.com/scrollmapper/bible_databases).

Usage:
    python3 build_bible_db.py path/to/bibles.sqlite KJV.json ASV.json BBE.json YLT.json

Each *.json argument is a file in scrollmapper's "formats/json" layout:
    { "translation": "<code>: <full name>", "books": [ {"name": ..., "chapters": [...] } ] }

Safe to re-run: each translation is fully replaced (old rows for that
code are deleted first), so re-importing an updated file just refreshes it.
"""
import json
import sqlite3
import sys
from pathlib import Path

SCHEMA = """
CREATE TABLE IF NOT EXISTS translations (
    code TEXT PRIMARY KEY,
    name TEXT NOT NULL,
    testament_order TEXT NOT NULL DEFAULT 'protestant'
);

CREATE TABLE IF NOT EXISTS books (
    translation_code TEXT NOT NULL REFERENCES translations(code) ON DELETE CASCADE,
    book_index INTEGER NOT NULL,   -- 0-based order within this translation
    name TEXT NOT NULL,
    PRIMARY KEY (translation_code, book_index)
);

CREATE TABLE IF NOT EXISTS verses (
    translation_code TEXT NOT NULL REFERENCES translations(code) ON DELETE CASCADE,
    book_index INTEGER NOT NULL,
    book_name TEXT NOT NULL,
    chapter INTEGER NOT NULL,
    verse INTEGER NOT NULL,
    text TEXT NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_verses_lookup
    ON verses(translation_code, book_index, chapter, verse);

CREATE INDEX IF NOT EXISTS idx_verses_book_name
    ON verses(translation_code, book_name, chapter, verse);

-- Full text search used by the panel's keyword-search box.
CREATE VIRTUAL TABLE IF NOT EXISTS verses_fts USING fts5(
    text,
    content='verses',
    content_rowid='rowid'
);

CREATE TRIGGER IF NOT EXISTS verses_ai AFTER INSERT ON verses BEGIN
    INSERT INTO verses_fts(rowid, text) VALUES (new.rowid, new.text);
END;
CREATE TRIGGER IF NOT EXISTS verses_ad AFTER DELETE ON verses BEGIN
    INSERT INTO verses_fts(verses_fts, rowid, text) VALUES('delete', old.rowid, old.text);
END;
CREATE TRIGGER IF NOT EXISTS verses_au AFTER UPDATE ON verses BEGIN
    INSERT INTO verses_fts(verses_fts, rowid, text) VALUES('delete', old.rowid, old.text);
    INSERT INTO verses_fts(rowid, text) VALUES (new.rowid, new.text);
END;
"""


def import_file(conn, path: Path):
    data = json.loads(path.read_text(encoding="utf-8"))
    full_name = data["translation"]
    code = full_name.split(":", 1)[0].strip()
    name = full_name.split(":", 1)[1].strip() if ":" in full_name else full_name

    cur = conn.cursor()
    cur.execute("DELETE FROM verses WHERE translation_code = ?", (code,))
    cur.execute("DELETE FROM books WHERE translation_code = ?", (code,))
    cur.execute("DELETE FROM translations WHERE code = ?", (code,))
    cur.execute("INSERT INTO translations(code, name) VALUES (?, ?)", (code, name))

    verse_rows = []
    for book_index, book in enumerate(data["books"]):
        cur.execute(
            "INSERT INTO books(translation_code, book_index, name) VALUES (?, ?, ?)",
            (code, book_index, book["name"]),
        )
        for chapter in book["chapters"]:
            chapter_num = chapter["chapter"]
            for v in chapter["verses"]:
                verse_rows.append(
                    (code, book_index, book["name"], chapter_num, v["verse"], v["text"])
                )

    cur.executemany(
        "INSERT INTO verses(translation_code, book_index, book_name, chapter, verse, text) "
        "VALUES (?, ?, ?, ?, ?, ?)",
        verse_rows,
    )
    conn.commit()
    print(f"Imported {code} ({name}): {len(verse_rows)} verses")


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)

    db_path = Path(sys.argv[1])
    conn = sqlite3.connect(db_path)
    conn.executescript(SCHEMA)

    for arg in sys.argv[2:]:
        import_file(conn, Path(arg))

    conn.execute("PRAGMA optimize;")
    conn.close()
    print(f"Done -> {db_path}")


if __name__ == "__main__":
    main()
