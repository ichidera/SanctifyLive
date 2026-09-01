# Changelog

## Unreleased

### Accurate live-appearance previews everywhere (`src/core/output/`, `src/core/theme/`, `src/core/song/`)

Every content-tab preview now shows *exactly* how a selection will appear
on the real congregation-facing screen -- correct aspect ratio, correct
cropping/cutoff, correct margins and font size for whatever resolution is
configured in Options -- instead of an unscaled thumbnail (Media), no
preview at all (Scriptures), or not existing yet (Songs, Themes).

**New shared rendering pipeline**
- `SlideRenderer` (new, `src/core/output/SlideRenderer.h/.cpp`): the one
  function that knows how to turn a `Slide` into pixels -- cover-fit
  background image or solid color, then centered word-wrapped text inset
  by the destination's configured margins, in the destination's
  configured font. Previously this logic was hand-rolled once inside
  `OutputWindow::paintEvent()` with hardcoded 40px margins and a font
  size with no connection to `OutputProfile` at all; margins and the
  output font in Options silently did nothing. `OutputWindow` now calls
  `SlideRenderer::paint()` too, so Options' margins/font finally take
  effect on the real output, not just in previews.
- `LiveAppearancePreview` (new, `src/core/output/LiveAppearancePreview.h/.cpp`):
  reusable preview widget. Renders into a box that always has the
  *destination's actual configured aspect ratio*
  (`OutputProfile::outputPosition`), letterboxed/pillarboxed to fit
  whatever space the widget has -- so anything that would be cropped or
  cut off on the real screen is cropped/cut off in the preview too,
  rather than a differently-shaped panel silently hiding it. Shows a
  resolution badge (e.g. "1920x1080") so it's always clear which output
  a given preview represents.
- `OutputProfile` changes (resolution, margins, font) now propagate live:
  `SettingsWindow` gained a `mainOutputProfileChanged(const OutputProfile&)`
  signal (emitted on OK, alongside the existing `mainOutputChanged`).
  `OperatorWindow` forwards it to the real `OutputWindow` and both its
  in-app mirrors, and to `MediaLibraryPanel`, which fans it out to every
  content tab's preview.
- Settings' own small preview thumbnail (`OutputPreviewThumbnail`) no
  longer draws a fixed mockup box -- it renders a representative sample
  slide through the same `SlideRenderer`, using the `OutputProfile` of
  whichever category is currently open, so margin/font/resolution
  changes are visible there immediately too.

**Media tab**
- Replaced the static `QLabel`-based preview (an unscaled/uncropped
  thumbnail) with `LiveAppearancePreview`. Selecting an entry shows
  precisely how its cover-fit crop and focus point will look on the
  actual configured output.
- `Slide::fromMediaEntry()` (new, `src/core/schedule/Slide.h`): the one
  place a media-library-style entry (label, color, image path, focus
  point) becomes a `Slide`. Used by the Media preview, the Themes
  preview, and `OperatorWindow::onMediaActivated` -- so what's
  previewed and what actually goes live can never diverge.

**Scriptures tab**
- Added a live-appearance preview pane (there wasn't one before -- see
  the previous entry's note that this panel "does not implement the live
  preview"). Selecting verse(s) in the list drives it immediately.
- `composeProjectedScripture()` (new,
  `src/core/scripture/ScriptureFormatting.h`): the one function that
  builds "verse text + Reference (TRANSLATION)" footnote. Both the
  preview and `OperatorWindow::onScriptureActivated` (the real "Send
  Selected" path) call it, so the preview always matches what's
  actually sent.

**Songs tab (new)** -- was a disabled placeholder with no underlying
code at all.
- `SongPanel` (new, `src/core/song/`): an in-memory song library (title
  + lyrics, `SongEditorDialog` for add/edit/delete). Lyrics are split
  into slides on blank lines, further broken up per Options → Main
  Output → Song → "Max lines per slide," and title-prefixed if "Show
  song title" is enabled -- both settings were previously stored but
  never actually used anywhere. Slide list and preview are driven by the
  exact same composition function, so what's previewed is what gets sent
  when a slide is double-clicked.
- Ships with one public-domain sample hymn ("Amazing Grace") to
  demonstrate the layout, same role Media's built-in swatches play.

**Themes tab (new)** -- was also a disabled placeholder with no code.
- `ThemePanel` (new, `src/core/theme/`): a small built-in library of
  solid-color background themes, plus "+" to add a custom solid-color or
  image theme. Uses the same `LiveAppearancePreview` +
  `Slide::fromMediaEntry()` as Media, and sends live through
  `OperatorWindow::onMediaActivated` (reusing that slot directly, since
  a theme is just a full-screen background like a Media entry).

**Verified**: installed a Qt6 dev toolchain and CMake in the build
sandbox and did a full clean build (`cmake` configure + `make -j`) --
zero errors and zero warnings under `-Wall -Wextra`. Also smoke-tested
that the app launches and constructs its whole UI (including all four
new/changed preview panes) without crashing under
`QT_QPA_PLATFORM=offscreen`. Full interactive/visual verification (the
actual look of each preview against a real screen) still needs a normal
desktop run, since this sandbox has no display.

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
