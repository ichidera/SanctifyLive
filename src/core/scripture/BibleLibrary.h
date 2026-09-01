#ifndef SANCTIFYLIVE_CORE_SCRIPTURE_BIBLELIBRARY_H_
#define SANCTIFYLIVE_CORE_SCRIPTURE_BIBLELIBRARY_H_


#include <QSqlDatabase>
#include <QString>
#include <QStringList>
#include <QVector>

// BibleLibrary loads Scripture text out of a small SQLite database
// (resources/scripture/bibles.sqlite) and exposes it as plain C++
// types the ScripturePanel UI can bind to, without any Qt SQL model
// classes leaking into the UI layer.
//
// The database is built offline by scripts/build_bible_db.py from
// translation files in scrollmapper's bible_databases JSON format
// (https://github.com/scrollmapper/bible_databases/tree/master/formats/json),
// which packages a large number of public-domain / permissively-
// licensed translations. Four public-domain English translations ship
// built in (KJV, ASV, BBE, YLT); more can be added by downloading
// additional *.json files from that repo and re-running the script --
// see resources/scripture/README.md.
//
// Schema (see scripts/build_bible_db.py for the authoritative DDL):
//   translations(code, name)
//   books(translation_code, book_index, name)
//   verses(translation_code, book_index, book_name, chapter, verse, text)
//   verses_fts -- FTS5 index over verses.text, used by search()
class BibleLibrary
{
public:
    BibleLibrary();
    ~BibleLibrary();

    // Tries a short list of candidate locations (installed-next-to-exe,
    // and a development fallback under the source tree) and opens the
    // first one found. Returns false (see lastError()) if none exist or
    // the file couldn't be opened as a valid database.
    bool openDefault();
    bool openFile(const QString &sqlitePath);
    bool isOpen() const;
    QString lastError() const { return m_lastError; }

    struct Translation
    {
        QString code; // e.g. "KJV"
        QString name; // e.g. "King James Version (1769) ..."
    };
    // In insertion order (roughly: build order in bibles.sqlite).
    QVector<Translation> translations() const;

    // Book names in canonical order for this translation (normally 66
    // entries: 39 Old Testament + 27 New Testament).
    QStringList bookNames(const QString &translationCode) const;
    int oldTestamentBookCount(const QString &translationCode) const;

    int chapterCount(const QString &translationCode, const QString &bookName) const;
    int verseCount(const QString &translationCode, const QString &bookName, int chapter) const;

    // Bounds checks used by the search box to reject a chapter/verse
    // number the moment it becomes impossible, rather than after the
    // fact. chapter/verse <= 0 is always invalid (there is no "chapter
    // 0"); chapterCount()/verseCount() return 0 for an unknown book, so
    // these also safely reject a not-yet-resolved book.
    bool isValidChapter(const QString &translationCode, const QString &bookName, int chapter) const;
    bool isValidVerse(const QString &translationCode, const QString &bookName, int chapter, int verseNum) const;

    // Returns every book name (in canonical order) whose name -- or a
    // recognized abbreviation for it -- starts with `prefix`. This is
    // what powers the search box's autocomplete: prefix "g" matches
    // both "Genesis" and "Galatians"; "ge" narrows to just "Genesis".
    // Empty prefix or no match returns an empty list; the caller decides
    // what an empty result means (e.g. "not a book -- try a keyword
    // search instead").
    QStringList matchBookNames(const QString &translationCode, const QString &prefix) const;

    struct Verse
    {
        QString book;
        int chapter = 0;
        int verse = 0;
        QString text;
    };
    QVector<Verse> versesInChapter(const QString &translationCode, const QString &bookName, int chapter) const;
    bool verseText(const QString &translationCode, const QString &bookName, int chapter, int verseNum,
                   QString *outText) const;

    struct ParsedReference
    {
        bool valid = false;
        QString book;    // canonical book name as stored in this translation
        int chapter = 0;
        int verseStart = 0; // 0 means "whole chapter, no specific verse"
        int verseEnd = 0;   // equals verseStart for a single verse
    };
    // Parses things like "John 3:16", "1 John 3:16-18", "Gen 1", "Ps 23",
    // "Song of Solomon 2:1". Book-name matching is case-insensitive and
    // understands common abbreviations; falls back to a prefix match
    // against this translation's book list. Returns valid=false (with no
    // exception) for anything that doesn't look like a reference at all,
    // so callers can fall back to treating the same text as a keyword
    // search.
    ParsedReference parseReference(const QString &translationCode, const QString &input) const;

    struct SearchResult
    {
        QString book;
        int chapter = 0;
        int verse = 0;
        QString text;
    };
    // Keyword search via the FTS5 index. `query` is split on whitespace
    // and every term is required to appear (AND), which keeps results
    // relevant without the user needing to know FTS query syntax.
    QVector<SearchResult> search(const QString &translationCode, const QString &query, int limit = 200) const;

    // Path to the currently-open sqlite file (empty if none is open).
    // Exposed so callers that need to *write* to it (importing a new
    // translation) know what file to open a second, writable connection
    // against -- see importTranslationFromJsonFile().
    QString databasePath() const { return m_dbPath; }

    // Imports one translation from a scrollmapper bible_databases-format
    // JSON file (https://github.com/scrollmapper/bible_databases/tree/
    // master/formats/json) into the currently-open database, replacing
    // any existing translation with the same code (same behavior as
    // scripts/build_bible_db.py, just from a live app instead of the
    // offline tool). Used by both "Add Bible from disk..." (a local
    // file the person already has) and the "More available" download
    // flow (a file fetched from the repository first, then imported the
    // same way). On success, re-opens the database so the new
    // translation is immediately visible via translations()/etc.
    //
    // If the current database file isn't writable (e.g. installed under
    // a read-only Program Files-style directory), transparently copies
    // it to a per-user writable location first and switches to that
    // copy for this and future imports -- the caller doesn't need to
    // know or care which path ends up being used.
    bool importTranslationFromJsonFile(const QString &jsonPath, QString *outCode = nullptr,
                                        QString *outError = nullptr);

private:
    QString findDefaultDatabasePath() const;
    QString normalizeBookNameLookup(const QString &translationCode, const QString &rawBookName) const;
    bool ensureWritableDatabasePath(QString *outError);

    QSqlDatabase m_db;
    QString m_connectionName;
    QString m_lastError;
    QString m_dbPath;
};

#endif // SANCTIFYLIVE_CORE_SCRIPTURE_BIBLELIBRARY_H_
