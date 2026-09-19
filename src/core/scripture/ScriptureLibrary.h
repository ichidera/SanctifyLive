#pragma once

#include <QString>
#include <QVector>

// One verse in a loaded translation, flattened out of the bundled
// JSON's nested book -> chapter -> verse structure into a single,
// directly-indexable list -- the natural shape for a scrolling
// reference table (see ui/ScripturePanel) and for linear canonical-
// order search/lookup.
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
// "John 3:99" without scanning the whole translation on every
// keystroke. Computed independently per translation, since two
// translations of the same 66-book canon can still disagree on exactly
// where a verse boundary falls (a translator choosing to split or merge
// a sentence differently) -- NHEB has two more verses total than KJV
// for exactly this reason, even though both cover the same books.
struct ScriptureBookInfo
{
    QString name;
    int firstVerseRow = 0;         // row into verses(code) of this book's Chapter 1, Verse 1
    QVector<int> versesPerChapter; // versesPerChapter[c-1] = verse count of chapter c
    int chapterCount() const { return versesPerChapter.size(); }
};

// One bundled translation: its short app-facing code (what
// ScripturePanel's translation list and QSettings both use), a
// human-readable name for tooltips, and its language.
struct ScriptureTranslation
{
    QString code;        // "KJV", "ASV", "NHEB", "RVA" -- stable, used as a dictionary key
    QString displayName; // "King James Version (1769)", etc -- for tooltips/labels
    QString language;    // BCP-47-ish tag for the translation's language: "en", "es"
};

// ScriptureLibrary loads and caches every bundled translation (see
// availableTranslations() for the current list) from Qt resources
// embedded at build time (see resources/Scriptures.qrc), so they're
// always available regardless of the app's working directory or
// install layout -- unlike Media tab imports
// (core/storage/MediaLibraryStore), which deliberately reference
// external files the operator picked, this is bundled content the app
// ships with.
//
// Kept in core/ (no Qt widgets) so it stays reusable from a future
// headless mode, same rationale as ScheduleIO/MediaLibraryStore.
//
// ---------------------------------------------------------------------
// A note on which translations are (and are NOT) here, for whoever
// adds the next one:
//
// Every translation bundled here is in the public domain in the United
// States, meaning SanctifyLive can embed the full text in every copy of
// the app, forever, at no cost and with no permission needed:
//   KJV  -- King James Version (1769 Blayney standardization). The
//           original 1611 translation's crown copyright has long since
//           expired outside the UK, and the 1769 text is the one almost
//           every "KJV" data source (this one included) actually uses.
//   ASV  -- American Standard Version (1901). A more literal, formal
//           sibling of the KJV tradition; its copyright was never
//           renewed and it has been public domain for decades.
//   NHEB -- New Heart English Bible. A modern-English public-domain
//           translation -- the closest thing on offer here to the
//           reading level of something like the NIV, without NIV's
//           copyright.
//   RVA  -- Reina-Valera Antigua (1909 revision). The 1909 revision
//           specifically is public domain; note that the far more
//           common "Reina-Valera 1960" (RVR60) most Spanish-speaking
//           congregations actually use day-to-day is NOT -- it's
//           copyrighted by the United Bible Societies/Sociedades
//           Bíblicas Unidas.
//
// Deliberately NOT bundled: NIV and HCSB/CSB. Both are actively
// copyrighted and trademarked (NIV by Biblica, licensed via Zondervan;
// HCSB/CSB by Holman Bible Publishers/Lifeway) and require a paid
// licence to redistribute at the scale of "every copy of this app ships
// the full text" -- OpenLP has documented hitting the same wall for the
// same reason ("Bible societies require exhorbitant licensing costs
// which OpenLP could never afford"). A future Store is the right way to
// offer those: fetched per-user under whatever licence terms the
// publisher requires, not embedded here for everyone.
// ---------------------------------------------------------------------
namespace ScriptureLibrary
{

// Every translation bundled with this build, in the order they should
// be listed. Pure metadata -- doesn't load anything.
const QVector<ScriptureTranslation> &availableTranslations();

// Loads (once per translation code; cached thereafter) and returns
// every verse in that translation, in canonical Bible order (Genesis
// 1:1 first, Revelation last). Returns an empty list (logged, not
// fatal) if `translationCode` isn't in availableTranslations() or its
// embedded resource is somehow missing or malformed -- the Scriptures
// tab should degrade to "no verses found", never crash the app.
const QVector<ScriptureVerse> &verses(const QString &translationCode);

// That translation's table of contents, derived from verses() once and
// cached -- same "load once, reuse forever" reasoning.
const QVector<ScriptureBookInfo> &books(const QString &translationCode);

// Exact-match lookup of a verse's row in verses(translationCode), or -1
// if that book/chapter/verse doesn't exist in that translation. `book`
// is matched case-insensitively against the canonical name (no alias
// resolution here -- see core/scripture/ScriptureReference for
// alias-aware book matching from free-typed input).
int rowForReference(const QString &translationCode, const QString &book, int chapter, int verse);

} // namespace ScriptureLibrary
