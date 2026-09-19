#pragma once

#include <QString>
#include <QVector>

#include "core/song/Song.h"

// Parses and renders the plain-text bracket-tag convention the Songs
// tab's editor uses for entering lyrics (see ui/SongEditorDialog) --
// the same style of markup ChordPro/OpenSong-style tools use, so
// someone who has typed up lyrics for one of those before doesn't have
// to learn a new convention:
//
//   [Verse 1]
//   Amazing grace, how sweet the sound
//   That saved a wretch like me
//
//   [Chorus]
//   It is well, with my soul
//
//   [Verse 2]
//   ...
//
// A bracket tag names a section (Verse/Chorus/Bridge/Pre-Chorus/Intro/
// Ending, case-insensitive, optionally followed by a number -- "Chorus"
// alone means "Chorus 1"); every line until the next bracket tag (or
// end of text) is that section's lyrics. Kept in core/ (no Qt widgets)
// so the format itself is testable independent of the editor UI that
// happens to expose it today.
namespace SongTextFormat
{

// Parses `text` into `outSections`, returning true if at least one
// bracket-tagged section was found. Text before the first bracket tag
// is ignored (treated as accidental leading whitespace/notes, not an
// untitled section) rather than rejected -- a stray blank line at the
// top of a pasted-in song shouldn't break the whole import.
bool parse(const QString &text, QVector<SongSection> *outSections);

// The inverse of parse(): renders `sections` back into the same
// bracket-tagged text, for editing an existing song (SongEditorDialog
// loads a song by rendering it back to text, then re-parses on save).
QString render(const QVector<SongSection> &sections);

} // namespace SongTextFormat
