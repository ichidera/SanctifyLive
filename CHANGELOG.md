# Changelog

All notable changes to SanctifyLive are documented in this file.

The format loosely follows [Keep a Changelog](https://keepachangelog.com/),
and versioning follows [Semantic Versioning](https://semver.org/) — this
project is pre-1.0, so minor bumps may still include breaking changes.

## [0.4.0] - 2026-09-16

### Added

- **Live Captions display** — the TRANSCRIPTION panel (bottom-right of
  the Lower Area, below History) now has a real, working display
  instead of a bare disabled-button placeholder.
  - New `src/ui/LiveCaptionsPanel.{h,cpp}`: a read-only, word-wrapping,
    auto-scrolling `QPlainTextEdit`-backed widget with a small public
    surface — `appendCaption(text)` adds one finalized line and scrolls
    it into view; `clear()` resets to the placeholder. Whitespace-only
    text is ignored rather than adding a blank line.
  - Capped at 500 caption lines (oldest trimmed first) so a multi-hour
    service doesn't grow the widget's document unbounded — there's no
    transcript-archive feature (unlike History, which is deliberately
    append-only forever).
  - Styled to match the app's panel look (`Theme.cpp`:
    `QPlainTextEdit#liveCaptionsView`) rather than falling back to the
    plain default widget background.
  - Wired into `OperatorWindow::buildTranscriptionPanel()` in place of
    the old static "Not implemented yet" label. **This is a display
    only** — there's still no audio capture or speech-to-text engine
    anywhere in the project (see README roadmap), so Start Transcription
    stays an honest disabled stub. `appendCaption()` is the entire
    surface a future STT pipeline would call into.

### Verification

- Full clean CMake + Qt6 rebuild, `-Wall -Wextra`, zero warnings.
- Headless app launch (`QT_QPA_PLATFORM=offscreen`), no crash.
- Standalone behavioral test of `LiveCaptionsPanel` (built with a
  manually-generated moc, since `Q_OBJECT` needs it outside the CMake
  build): whitespace-only input is dropped, real captions accumulate
  and are readable back, pushing 600 lines through leaves at most 500
  with the oldest ("line 0") gone and the newest ("line 599") present,
  and `clear()` empties the document. All passed.

### Not done in this iteration (see README roadmap / "we'd add other features")

- No audio capture, no speech-to-text engine, no Start/Stop wiring —
  `appendCaption()` has nothing calling it yet.
- No interim/partial-result handling, speaker labels, timestamps, or
  transcript export. Deliberately not guessing at an API shape for
  these before a real STT engine exists to define what it actually
  needs.

## [0.3.1] - 2026-09-16

### Fixed

- **Media tab tile grid no longer leaves a dead strip of unused space.**
  Two related layout bugs, both in `src/ui/MediaLibraryPanel`:
  - The folder tree (Videos/Images) had a *max* width, not a fixed one,
    so the splitter was free to give it more room than its two short
    labels ever needed — showing up as empty space to its right.
    `m_tree` is now `setFixedWidth(130)`, and the tree|grid `QSplitter`
    gets explicit initial `setSizes({130, 5000})` (the same
    first-layout-ignores-stretch-factors fix already used for
    `OperatorWindow`'s Lower Area splitter — stretch factors only take
    effect on *subsequent* resizes, not the initial layout pass).
  - Tiles were placed at a fixed size with no reflow, so widening the
    panel just grew the right-hand margin instead of using the space.
    `MediaFolder` now tracks its tiles directly (`QVector<QWidget*>
    tiles`, replacing the old `nextRow`/`nextCol` grid cursor), and a
    new `relayoutFolder()` recomputes how many equal-width columns fit
    the folder's current viewport width and stretches every tile to
    fill it, so the grid always ends flush with the right edge. It's
    called on every `addTile()` and, via a `QEvent::Resize` event filter
    installed on each folder's `QScrollArea` viewport, whenever that
    width actually changes (a viewport can resize independent of this
    panel's own size, e.g. a scrollbar appearing/disappearing).

### Changed

- `MediaLibraryPanel::MediaFolder::page` is now `QScrollArea *` instead
  of `QWidget *`, since `relayoutFolder()` and the new event filter need
  `->viewport()`, which only `QAbstractScrollArea` subclasses expose.

### Merge notes

- This landed as a pasted full rewrite of `MediaLibraryPanel.cpp`
  without the persistent-storage work from 0.3.0. Diffed it in,
  confirmed the parts that mattered (folder/tile structure, splitter
  setup, the new reflow logic) were unrelated to storage, then
  re-threaded `loadPersistedMedia()` / `persistImportedMedia()` through
  the new `addTile()` call sites (same call sites as before — the
  signature didn't change) and updated the header for the new
  `MediaFolder` shape (`tiles` list instead of `nextRow`/`nextCol`) and
  the new `relayoutFolder()` / `eventFilter()` declarations.
- Verified with a full clean rebuild (`-Wall -Wextra`, zero warnings)
  and a headless launch (`QT_QPA_PLATFORM=offscreen`) with no crash.

## [0.3.0 merge pass] - re-applied storage feature, no functional change

### Merged

- Re-applied the persistent Media library feature (below) onto a fresh
  copy of the project. Diffed the newly uploaded copy against the
  pre-storage baseline first: the only files that differed from that
  baseline were the same four files the storage feature itself touches
  (`CMakeLists.txt`, `README.md`, `src/ui/MediaLibraryPanel.{h,cpp}`),
  and after reversing the storage edits those matched byte-for-byte too
  — so there was nothing of yours to reconcile against; this pass is
  the storage feature applied cleanly on top of your upload as-is.
  Rebuilt clean with `-Wall -Wextra`, zero warnings, to confirm.

## [0.3.0] - 2026-09-15

### Added

- **Persistent Media library** — imported images and videos in the Media
  tab now survive an app restart. Previously, every image/video an
  operator added via the Media tab's **+** button lived only in memory:
  closing and reopening SanctifyLive reset the library back to the six
  built-in Images swatches and an empty Videos folder, even though the
  original files were untouched on disk. The app just had no memory of
  ever having seen them.
  - New module: `src/core/storage/MediaLibraryStore.{h,cpp}` — a small,
    Qt-widget-free JSON store (same design as `core/model/ScheduleIO`)
    that remembers `(name, file path, kind)` for each imported item.
    Lives at `<AppDataLocation>/media_library.json` (e.g.
    `~/.local/share/SanctifyLive/SanctifyLive/media_library.json` on
    Linux), not next to the executable — this is small private
    bookkeeping, not something an operator needs to see or move around.
  - **Files are referenced, not copied.** The store only remembers where
    an operator pointed at (e.g. a shared drive or USB stick), the same
    way `Slide::backgroundImagePath` already worked. Silently
    duplicating potentially large video files into app-owned storage
    every time someone clicks **+** was judged the wrong default for a
    volunteer-run tool; this can be revisited later as an explicit "copy
    into library" option if operators want it.
  - **Self-healing on load.** If a remembered file has since been moved,
    deleted, or is on a USB stick that isn't currently plugged in,
    `MediaLibraryPanel` drops that entry instead of showing a broken
    tile, logs a warning, and rewrites the store so the same phantom
    entry doesn't keep coming back every launch. The same check runs for
    images that exist but no longer decode (e.g. a corrupted file).
  - `MediaLibraryPanel` now loads the store once at startup (after the
    built-in swatches are in place) and appends-and-saves after every
    successful import — so a crash or force-quit right after importing
    still only costs, at most, that one item.
  - Built-in Images swatches are intentionally never written to the
    store — only genuinely imported files are persisted, so the catalog
    on disk always maps to real files an operator chose.

### Changed

- `CMakeLists.txt`: added `src/core/storage/` to the build and to the
  top-of-file directory-layout doc comment; version bumped 0.2.2 → 0.3.0.

### Verification

- Full CMake + Qt6 rebuild from clean (`-Wall -Wextra`) with zero
  warnings.
- App launches headlessly (`QT_QPA_PLATFORM=offscreen`) with no store
  file present (first-run path) without error.
- Standalone round-trip test of `MediaLibraryStore::save`/`load`:
  multi-entry save/reload, image + video kinds, and the "no file on
  disk yet" case (must return success with an empty list, not an
  error).

### Not done in this iteration (see README roadmap)

- Video is still catalog-only — persistence covers *that a video was
  imported*, not playback or thumbnails.
- No "copy into app storage" option yet, so a persisted entry can still
  go missing if the source file is moved or a removable drive is
  unplugged (handled gracefully, per above, but the file itself isn't
  backed up).
- No dedicated Settings UI to view/clear the persisted library or
  relocate a moved file — that lives in the disabled Media-tab Settings
  stub for now.

## [0.2.2] - prior to this changelog's introduction

Baseline snapshot at the time this changelog was added: operator
console layout, Preview/Live/History, save/open schedules
(`core/model/ScheduleIO`), resolution-independent rendering, and the
Media tab's real (but non-persistent) image import / video cataloging.
See `README.md` for the full feature description as of this version.
