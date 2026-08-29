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

private:
    QString findDefaultDatabasePath() const;
    QString normalizeBookNameLookup(const QString &translationCode, const QString &rawBookName) const;

    QSqlDatabase m_db;
    QString m_connectionName;
    QString m_lastError;
};

#endif // SANCTIFYLIVE_CORE_SCRIPTURE_BIBLELIBRARY_H_
