#pragma once

#include <QString>
#include <QVector>

// One verse in a loaded translation, flattened out of KJV.json's nested
// book -> chapter -> verse structure into a single, directly-indexable
// list -- the natural shape for a scrolling reference table (see
// ui/ScripturePanel) and for linear canonical-order search/lookup.
struct ScriptureVerse
{
    QString book;
    int chapter = 0;
    int verse = 0;
    QString text;
    QString reference; // precomputed "Book Chapter:Verse", e.g.
                        // "Genesis 1:1" -- so the UI never has to
                        // reformat this on every row of a 31,000+ row
                        // table.
};

// Per-book table of contents: how many chapters a book has, and how
// many verses each of those chapters has -- exactly what the guided
// Book -> Chapter -> Verse reference navigator (see
// core/scripture/ScriptureReference) needs to reject "John 999" or
// "John 3:99" without scanning the whole Bible on every keystroke.
struct ScriptureBookInfo
{
    QString name;
    int firstVerseRow = 0;         // row into verses() of this book's Chapter 1, Verse 1
    QVector<int> versesPerChapter; // versesPerChapter[c-1] = verse count of chapter c
    int chapterCount() const { return versesPerChapter.size(); }
};

// ScriptureLibrary loads and caches the one bundled translation (KJV
// today -- see ui/ScripturePanel's doc comment for why only one ships
// right now) from the Qt resource embedded at build time (see
// resources/Scriptures.qrc), so it's always available regardless of
// the app's working directory or install layout -- unlike Media tab
// imports (core/storage/MediaLibraryStore), which deliberately
// reference external files the operator picked, this is bundled
// content the app ships with.
//
// Kept in core/ (no Qt widgets) so it stays reusable from a future
// headless mode, same rationale as ScheduleIO/MediaLibraryStore.
namespace ScriptureLibrary
{

// Short display code for the one bundled translation, e.g. "KJV".
QString translationCode();

// Loads (once; cached thereafter) and returns every verse in the
// bundled translation, in canonical Bible order (Genesis 1:1 first,
// Revelation last). Returns an empty list (logged, not fatal) if the
// embedded resource is somehow missing or malformed -- the Scriptures
// tab should degrade to "no verses found", never crash the app.
const QVector<ScriptureVerse> &verses();

// The bundled translation's table of contents, derived from verses()
// once and cached -- same "load once, reuse forever" reasoning.
const QVector<ScriptureBookInfo> &books();

// Exact-match lookup of a verse's row in verses(), or -1 if that
// book/chapter/verse doesn't exist in the bundled translation. `book`
// is matched case-insensitively against the canonical name (no alias
// resolution here -- see core/scripture/ScriptureReference for
// alias-aware book matching from free-typed input).
int rowForReference(const QString &book, int chapter, int verse);

} // namespace ScriptureLibrary
