#pragma once

#include <QString>
#include <QVector>

// Implements the "progressively constrained Bible reference navigator"
// used by the Scriptures tab's Reference search mode (see
// ui/ScripturePanel's doc comment for the full rationale). The
// grammar is Book -> Chapter -> Verse -> optional end-Verse, and each
// stage only makes sense once the one before it has resolved -- you
// can't type a verse number before a chapter is known, because "verse
// 16" means nothing without knowing which chapter's verse 16.
//
// Pure logic over ScriptureLibrary's data, no Qt widgets, so it's
// unit-testable and reusable from a future headless mode -- same
// rationale as the rest of core/.
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
    int endVerse = 0;     // 0 = no range requested
    bool valid = false;   // true once Book+Chapter+Verse resolve to a real verse
    bool bookAmbiguous = false; // typed text matches more than one book, none of them exactly
    QString error;        // human-readable problem (e.g. out-of-range chapter); empty if none
    int firstRow = -1;    // row into ScriptureLibrary::verses() of the starting verse
    int lastRow = -1;     // row of the range's last verse (== firstRow if no range)
};

// Parses free-typed input against the Book -> Chapter -> Verse[-Verse]
// grammar described above. Never throws and never partially crashes on
// garbage input -- worst case, `valid` stays false and `stage` reports
// where parsing got stuck (e.g. still Stage::Book because nothing typed
// so far matches a real book).
Parsed parse(const QString &input);

struct BookSuggestion
{
    QString name;           // canonical book name to display
    QString completedInput; // what the search box should become if this is picked
};

// Book-name suggestions for the given (partial) book query, exact
// matches first, then prefix matches in canonical Bible order. Only
// meaningful while parsing is still stuck at Stage::Book.
QVector<BookSuggestion> bookSuggestions(const QString &bookQuery, int maxResults = 8);

} // namespace ScriptureReference
