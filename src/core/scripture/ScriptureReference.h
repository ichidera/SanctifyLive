#pragma once

#include <QString>
#include <QVector>

// Implements the "progressively constrained Bible reference navigator"
// used by the Scriptures tab's Reference search mode (see
// ui/ScripturePanel's doc comment for the full rationale, and its
// research notes on EasyWorship/OpenLP's real-world equivalents). The
// grammar is Book -> Chapter -> Verse -> optional end-Verse (optionally
// itself in a different chapter), and each stage only makes sense once
// the one before it has resolved -- you can't type a verse number
// before a chapter is known, because "verse 16" means nothing without
// knowing which chapter's verse 16.
//
// Supported reference formats (matching OpenLP's documented Scripture
// Reference syntax, since it's a reasonable, already-field-tested
// grammar rather than one invented from scratch):
//   Book                      -- e.g. "John"
//   Book Chapter              -- e.g. "John 3"
//   Book Chapter:Verse        -- e.g. "John 3:16" (a space instead of
//                                the colon works too -- "John 3 16" --
//                                matching EasyWorship's documented
//                                Quick Search flow of pressing Spacebar
//                                between book/chapter/verse instead of
//                                typing punctuation)
//   Book Chapter:Verse-Verse  -- e.g. "John 3:16-18"
//   Book Chapter:Verse-Chapter:Verse -- e.g. "John 3:16-4:2" (crosses a
//                                chapter boundary)
//
// Pure logic over ScriptureLibrary's data, no Qt widgets, so it's
// unit-testable and reusable from a future headless mode -- same
// rationale as the rest of core/. Parameterized by translation code
// because two translations of the same 66-book canon can still disagree
// on exactly where a verse boundary falls (see ScriptureLibrary.h) --
// "John 3" has to be validated against whichever translation is
// actually about to be searched, not a single hardcoded chapter/verse
// count.
namespace ScriptureReference
{

enum class Stage
{
    Book,    // still typing/choosing a book name
    Chapter, // book resolved; typing a chapter number
    Verse,   // book + chapter resolved; typing a starting verse number
    Range,   // book + chapter + verse resolved; optionally typing an end verse
};

struct Parsed
{
    Stage stage = Stage::Book;
    QString bookQuery;    // raw text typed for the book so far, trimmed
    QString book;         // resolved canonical book name; empty if unresolved
    int chapter = 0;      // 0 = not resolved
    int startVerse = 0;   // 0 = not resolved
    int endChapter = 0;   // only differs from `chapter` for a cross-chapter range; 0 if no range
    int endVerse = 0;     // 0 = no range requested
    bool valid = false;   // true once Book+Chapter+Verse resolve to a real verse
    bool bookAmbiguous = false; // typed text matches more than one book, none of them exactly
    QString error;        // human-readable problem (e.g. out-of-range chapter); empty if none
    int firstRow = -1;    // row into ScriptureLibrary::verses(translationCode) of the starting verse
    int lastRow = -1;     // row of the range's last verse (== firstRow if no range)
};

// Parses free-typed input against the grammar described above, against
// `translationCode`'s actual chapter/verse counts. Never throws and
// never partially crashes on garbage input -- worst case, `valid` stays
// false and `stage` reports where parsing got stuck (e.g. still
// Stage::Book because nothing typed so far matches a real book).
Parsed parse(const QString &translationCode, const QString &input);

struct BookSuggestion
{
    QString name;           // canonical book name to display
    QString completedInput; // what the search box should become if this is picked
};

// Book-name suggestions for the given (partial) book query, exact
// matches first, then prefix matches in canonical Bible order. Only
// meaningful while parsing is still stuck at Stage::Book. Book names
// are the same across every bundled translation (they're all the same
// 66-book Protestant canon), so this doesn't need a translation code.
QVector<BookSuggestion> bookSuggestions(const QString &bookQuery, int maxResults = 8);

struct BookNameVariant
{
    QString displayText;   // the alias as it would appear in normal writing, e.g. "1 John", "Revelation"
    QString canonicalName; // the book this resolves to, e.g. "I John", "Revelation of John"
};

// Every recognized way a book might legitimately be written or spoken:
// each canonical name plus its known aliases (numbered-book variants
// like "1 John" for the data's "I John", and "Revelation" for
// "Revelation of John") -- the same alias set parse()/bookSuggestions()
// already understand, exposed here so ScriptureDetector's free-text
// scan can anchor on real book-name mentions without maintaining a
// second copy of this list that could drift out of sync.
QVector<BookNameVariant> allBookNameVariants();

} // namespace ScriptureReference
