# Changelog

## Unreleased

### Scriptures panel overhaul (`src/core/scripture/`)

Builds on top of the Options → Main Output → General monitor-selection
update (Main Output now follows the monitor chosen in Options, falling
back to "prefer a second screen" until Options has been opened once).

**Search bar**
- Replaced the plain `QLineEdit` search box with a new `ScriptureSearchEdit`
  widget that runs in two modes, decided fresh on every keystroke:
  - **Reference mode** — book-name autocomplete as you type ("g" suggests
    Genesis and Galatians; "ge" narrows to Genesis; you can still type the
    full name by hand). Once a book is unambiguous, the chapter and then
    verse can follow; each digit is validated live against the real
    chapter/verse counts for that book, so an impossible chapter or verse
    number is rejected the instant it's typed rather than accepted and
    corrected afterward.
  - **Word/sentence search mode** — the moment what's typed doesn't match
    the start of any book, the box becomes a free-text keyword search that
    runs live as you type.
- After Enter resolves a reference, the box switches to a "committed"
  display (matching the reference-chip + hamburger-menu look) offering
  View (Single Line / Word Wrap), Sort by (Ascending / Descending), and
  Refresh. Clicking back into the box, or typing over it, immediately
  drops it back into a plain editable search field.

**Translations**
- `BibleLibrary` gained `matchBookNames()` (the autocomplete engine above),
  `isValidChapter()` / `isValidVerse()`, and
  `importTranslationFromJsonFile()` for importing a scrollmapper
  bible_databases-format translation into the live database (falls back
  to a per-user writable copy if the bundled database is read-only).
- New **"More Available..."** dialog (`TranslationLibraryDialog`): every
  listed translation is genuinely free (sourced from the public-domain
  scrollmapper/bible_databases repository) and reads "Free" — no pricing.
  "Get" downloads and imports a translation directly; already-installed
  ones show "Installed" instead.
- The "+" button next to the translation selector now offers
  **"Add Bible from Disk..."** (import a local Bible JSON file) alongside
  simple New Folder / New Collection / New My Folder / New My Collection
  organizers.

**Not changed**
- The live/preview output pipeline (`scriptureActivated` → `OperatorWindow`)
  is untouched — this iteration only touches the Scriptures tab's own UI.

**Verification**
- Full clean CMake + `make` build with zero errors/warnings (Qt 6,
  `-DCMAKE_BUILD_TYPE=Debug`).
- Standalone regex unit-check of the live reference parser against ~20
  representative inputs (partial book names, ambiguous prefixes, numbered
  books, mid-typing chapter/verse, free-text queries).
- Built and confirmed the app launches and stays running under a virtual
  display (`xvfb-run`) with no startup crash.
