#pragma once

#include <QString>
#include <QVector>

// Scans free-form text (a line of live transcription, sermon notes,
// anything not typed into the Scriptures tab's own structured search
// box) for substrings that look like spoken-or-written Bible
// references -- "John 3:16", "turn to Romans chapter 8 verse 28",
// "1 Corinthians 13:4-7" -- and resolves each one against a
// translation. This is the building block behind "notice a Scripture
// reference was just spoken and put it on screen automatically",
// the core feature of AI sermon-following tools like Pewbeam.
//
// See ui/LiveCaptionsPanel for where this actually gets wired to text
// that (eventually) comes from a live speech-to-text engine --
// appendCaption() is already the single entry point such an engine
// would call, so hooking detection there means it starts working the
// moment real transcription exists, with no further plumbing.
//
// Deliberately NOT attempting semantic/paraphrase detection (recognizing
// that a description of the Prodigal Son is "probably Luke 15" without
// the speaker ever saying a reference) -- that needs real NLP/embedding
// infrastructure this project doesn't have, and guessing at an API
// shape for it before that exists would be speculation, not
// engineering. This module only finds references that were actually
// SPOKEN AS a reference, in some recognizable phrasing -- a real,
// useful, honestly-scoped slice of what a full "AI sermon assistant"
// would do, not a pretend version of the whole thing.
//
// Pure logic over ScriptureLibrary's data, no Qt widgets, so it's
// unit-testable independent of any UI or (future) audio pipeline --
// same rationale as the rest of core/.
namespace ScriptureDetector
{

struct Detection
{
    QString matchedPhrase; // the substring in the source text that triggered this, e.g. "John chapter 3 verse 16"
    int position = -1;     // character offset of matchedPhrase within the scanned text
    QString reference;     // resolved canonical reference, e.g. "John 3:16" or "John 3:16-18"
    QString text;          // the verse's (or verse range's, joined) text in `translationCode`
};

// Finds every recognizable reference mention in `text` and resolves
// each against `translationCode`, in the order they appear. A phrase
// that looks reference-shaped but doesn't resolve to a real verse (a
// bad chapter/verse number) is silently skipped rather than reported as
// a malformed detection -- the caller only ever sees things it could
// actually display.
//
// This is a pattern matcher, not a language model: a common word that
// happens to also be a short book name (Mark, Acts, Job, Luke, James,
// John, Titus...) immediately followed by a number can produce a false
// positive ("Mark 5 minutes late" reads the same as "Mark, chapter 5").
// That trade-off is inherent to matching on wording alone; a caller
// showing detections to an operator to confirm (rather than
// auto-projecting them unconditionally) is the mitigation, not a
// smarter regex.
QVector<Detection> scan(const QString &translationCode, const QString &text);

} // namespace ScriptureDetector
