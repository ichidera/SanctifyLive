# Scripture database

`bibles.sqlite` backs the **Scriptures** tab (`src/core/scripture/`). It's a
small SQLite file with one row per verse, an index for fast
book/chapter/verse lookups, and an FTS5 full-text index for the panel's
keyword search box.

## What's bundled

Four full, public-domain English translations ship built in:

| Code | Translation |
|------|-------------|
| KJV  | King James Version (1769) |
| ASV  | American Standard Version (1901) |
| BBE  | Bible in Basic English (1949/1964) |
| YLT  | Young's Literal Translation (1898) |

All four come from the public domain / freely-licensed side of
[scrollmapper/bible_databases](https://github.com/scrollmapper/bible_databases),
an MIT-licensed project that packages several hundred translations in a
handful of common formats (SQL, JSON, XML, CSV). This project only uses
the `formats/json` files as an import source; the actual data ends up in
`bibles.sqlite`, not the original JSON.

## Adding more translations

That repo has far more than these four -- including many non-English
translations. To add one:

1. Grab the translation's `*.json` file from
   <https://github.com/scrollmapper/bible_databases/tree/master/formats/json>
   (check that translation's own license note in that repo -- not every
   entry there is public domain).
2. Re-run the importer, pointing it at the existing database plus the
   new file(s):

   ```bash
   python3 scripts/build_bible_db.py resources/scripture/bibles.sqlite path/to/NEW_TRANSLATION.json
   ```

   This updates `bibles.sqlite` in place -- existing translations are
   left alone, and re-importing a translation that's already present
   just replaces it (safe to re-run).
3. Rebuild the app. `ScripturePanel` reads the translation list straight
   out of the database, so a new translation just shows up in the
   dropdown -- no code changes needed.

## Schema

See the `SCHEMA` string at the top of `scripts/build_bible_db.py` for the
authoritative DDL. In short:

- `translations(code, name)`
- `books(translation_code, book_index, name)` -- canonical book order per translation
- `verses(translation_code, book_index, book_name, chapter, verse, text)`
- `verses_fts` -- FTS5 index over `verses.text`, kept in sync via triggers

## Regenerating from scratch

```bash
python3 scripts/build_bible_db.py resources/scripture/bibles.sqlite \
    path/to/KJV.json path/to/ASV.json path/to/BBE.json path/to/YLT.json
```
