#include "BibleLibrary.h"

#include <algorithm>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QRegularExpression>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>

namespace {

// Common English book-name abbreviations -> canonical (KJV-style) name.
// This is intentionally generous (multiple spellings per book) since the
// point is to make the reference box forgiving, not to validate input.
// Matching against the abbreviation table happens after a raw
// exact/prefix match against the translation's own book list fails, so
// a translation with differently-spelled book names still works for
// full names -- this table is purely a shortcut for common shorthand.
const QMap<QString, QString> &abbreviationTable()
{
    static const QMap<QString, QString> table = [] {
        QMap<QString, QString> m;
        auto add = [&m](const QString &canonical, std::initializer_list<const char *> abbrevs) {
            for (const char *a : abbrevs)
                m.insert(QString::fromLatin1(a).toLower(), canonical);
        };
        add("Genesis", {"gen", "ge", "gn"});
        add("Exodus", {"exo", "exod", "ex"});
        add("Leviticus", {"lev", "le", "lv"});
        add("Numbers", {"num", "nu", "nm", "nb"});
        add("Deuteronomy", {"deut", "dt"});
        add("Joshua", {"josh", "jos"});
        add("Judges", {"judg", "jdg", "jg"});
        add("Ruth", {"rth", "ru"});
        add("1 Samuel", {"1 sam", "1sam", "1sa", "i sam", "1st samuel"});
        add("2 Samuel", {"2 sam", "2sam", "2sa", "ii sam", "2nd samuel"});
        add("1 Kings", {"1 kgs", "1kgs", "1ki", "i kings", "1st kings"});
        add("2 Kings", {"2 kgs", "2kgs", "2ki", "ii kings", "2nd kings"});
        add("1 Chronicles", {"1 chron", "1chron", "1chr", "i chron", "1st chronicles"});
        add("2 Chronicles", {"2 chron", "2chron", "2chr", "ii chron", "2nd chronicles"});
        add("Ezra", {"ezr"});
        add("Nehemiah", {"neh"});
        add("Esther", {"esth", "est"});
        add("Job", {"jb"});
        add("Psalms", {"psalm", "psa", "ps", "pslm"});
        add("Proverbs", {"prov", "pro", "prv"});
        add("Ecclesiastes", {"eccl", "eccles", "ecc"});
        add("Song of Solomon", {"song", "sos", "song of songs", "canticles"});
        add("Isaiah", {"isa", "is"});
        add("Jeremiah", {"jer"});
        add("Lamentations", {"lam"});
        add("Ezekiel", {"ezek", "eze"});
        add("Daniel", {"dan", "dn"});
        add("Hosea", {"hos"});
        add("Joel", {"jl"});
        add("Amos", {"am"});
        add("Obadiah", {"obad", "ob"});
        add("Jonah", {"jnh", "jon"});
        add("Micah", {"mic"});
        add("Nahum", {"nah", "na"});
        add("Habakkuk", {"hab"});
        add("Zephaniah", {"zeph", "zep"});
        add("Haggai", {"hag"});
        add("Zechariah", {"zech", "zec"});
        add("Malachi", {"mal"});
        add("Matthew", {"matt", "mt"});
        add("Mark", {"mrk", "mk", "mr"});
        add("Luke", {"luk", "lk"});
        add("John", {"joh", "jn", "jhn"});
        add("Acts", {"act"});
        add("Romans", {"rom", "ro"});
        add("1 Corinthians", {"1 cor", "1cor", "i cor", "1st corinthians"});
        add("2 Corinthians", {"2 cor", "2cor", "ii cor", "2nd corinthians"});
        add("Galatians", {"gal"});
        add("Ephesians", {"eph"});
        add("Philippians", {"phil", "php"});
        add("Colossians", {"col"});
        add("1 Thessalonians", {"1 thess", "1thess", "i thess", "1st thessalonians"});
        add("2 Thessalonians", {"2 thess", "2thess", "ii thess", "2nd thessalonians"});
        add("1 Timothy", {"1 tim", "1tim", "i tim", "1st timothy"});
        add("2 Timothy", {"2 tim", "2tim", "ii tim", "2nd timothy"});
        add("Titus", {"tit"});
        add("Philemon", {"philem", "phm"});
        add("Hebrews", {"heb"});
        add("James", {"jas", "jm"});
        add("1 Peter", {"1 pet", "1pet", "i pet", "1st peter"});
        add("2 Peter", {"2 pet", "2pet", "ii pet", "2nd peter"});
        add("1 John", {"1 jn", "1jn", "i john", "1st john"});
        add("2 John", {"2 jn", "2jn", "ii john", "2nd john"});
        add("3 John", {"3 jn", "3jn", "iii john", "3rd john"});
        add("Jude", {"jud"});
        add("Revelation", {"rev", "revelations", "the revelation"});
        return m;
    }();
    return table;
}

// Different translation sources spell numbered books differently -- the
// scrollmapper KJV/ASV/BBE/YLT files bundled here use "I John", "II
// Samuel", etc., while plenty of other sources (and most of what users
// will type) use "1 John", "2 Samuel". Rather than picking one style as
// "the" canonical form and hoping every future translation matches it,
// book-name comparisons go through this: lowercase, and rewrite a
// recognized leading ordinal word (1/i/1st/first, 2/ii/2nd/second,
// 3/iii/3rd/third) to a single digit. Two spellings of the same book
// then compare equal regardless of which style either side used.
QString canonicalizeOrdinalPrefix(const QString &input)
{
    static const QMap<QString, QString> ordinalMap = {
        {"1", "1"}, {"i", "1"}, {"1st", "1"}, {"first", "1"},
        {"2", "2"}, {"ii", "2"}, {"2nd", "2"}, {"second", "2"},
        {"3", "3"}, {"iii", "3"}, {"3rd", "3"}, {"third", "3"},
    };
    const QString trimmed = input.trimmed().toLower();
    static const QRegularExpression leadTokenPattern(QStringLiteral(R"(^(\S+)\s+(.*)$)"));
    const QRegularExpressionMatch m = leadTokenPattern.match(trimmed);
    if (m.hasMatch()) {
        const auto it = ordinalMap.constFind(m.captured(1));
        if (it != ordinalMap.constEnd())
            return it.value() + QLatin1Char(' ') + m.captured(2);
    }
    return trimmed;
}

// Mirrors scripts/build_bible_db.py's SCHEMA exactly, so a database
// imported into at runtime ends up byte-for-byte compatible with one
// built offline by that script. CREATE ... IF NOT EXISTS makes this
// safe to run against a database that already has the schema (the
// normal case) as well as a fresh, empty file (a from-scratch "Add
// Bible from disk" with no bundled database at all).
const char *const kBibleSchemaDdl = R"SQL(
CREATE TABLE IF NOT EXISTS translations (
    code TEXT PRIMARY KEY,
    name TEXT NOT NULL,
    testament_order TEXT NOT NULL DEFAULT 'protestant'
);
CREATE TABLE IF NOT EXISTS books (
    translation_code TEXT NOT NULL REFERENCES translations(code) ON DELETE CASCADE,
    book_index INTEGER NOT NULL,
    name TEXT NOT NULL,
    PRIMARY KEY (translation_code, book_index)
);
CREATE TABLE IF NOT EXISTS verses (
    translation_code TEXT NOT NULL REFERENCES translations(code) ON DELETE CASCADE,
    book_index INTEGER NOT NULL,
    book_name TEXT NOT NULL,
    chapter INTEGER NOT NULL,
    verse INTEGER NOT NULL,
    text TEXT NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_verses_lookup
    ON verses(translation_code, book_index, chapter, verse);
CREATE INDEX IF NOT EXISTS idx_verses_book_name
    ON verses(translation_code, book_name, chapter, verse);
CREATE VIRTUAL TABLE IF NOT EXISTS verses_fts USING fts5(
    text,
    content='verses',
    content_rowid='rowid'
);
CREATE TRIGGER IF NOT EXISTS verses_ai AFTER INSERT ON verses BEGIN
    INSERT INTO verses_fts(rowid, text) VALUES (new.rowid, new.text);
END;
CREATE TRIGGER IF NOT EXISTS verses_ad AFTER DELETE ON verses BEGIN
    INSERT INTO verses_fts(verses_fts, rowid, text) VALUES('delete', old.rowid, old.text);
END;
CREATE TRIGGER IF NOT EXISTS verses_au AFTER UPDATE ON verses BEGIN
    INSERT INTO verses_fts(verses_fts, rowid, text) VALUES('delete', old.rowid, old.text);
    INSERT INTO verses_fts(rowid, text) VALUES (new.rowid, new.text);
END;
)SQL";

} // namespace

BibleLibrary::BibleLibrary()
{
    // Each BibleLibrary instance gets its own uniquely-named QSqlDatabase
    // connection, since Qt's connection registry is a shared, name-keyed
    // global -- this avoids clashing with other database users in the
    // process (there are none today, but the panel shouldn't assume that).
    m_connectionName = QStringLiteral("bible_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

BibleLibrary::~BibleLibrary()
{
    if (m_db.isValid()) {
        m_db.close();
        m_db = QSqlDatabase();
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

QString BibleLibrary::findDefaultDatabasePath() const
{
    const QString relative = QStringLiteral("resources/scripture/bibles.sqlite");

    QStringList candidates;
    // 1. Installed/packaged layout: next to the executable.
    candidates << QDir(QCoreApplication::applicationDirPath()).filePath(relative);
    // 2. Common build-tree layout: executable in build/, resources at
    //    the project root one level up.
    candidates << QDir(QCoreApplication::applicationDirPath()).filePath("../" + relative);
    candidates << QDir(QCoreApplication::applicationDirPath()).filePath("../../" + relative);
#ifdef SANCTIFYLIVE_SOURCE_DIR
    // 3. Development fallback: run straight from the source tree's build
    //    directory without any install/copy step.
    candidates << QDir(QStringLiteral(SANCTIFYLIVE_SOURCE_DIR)).filePath(relative);
#endif

    for (const QString &candidate : candidates) {
        if (QFileInfo::exists(candidate))
            return QDir::cleanPath(candidate);
    }
    return QString();
}

bool BibleLibrary::openDefault()
{
    const QString path = findDefaultDatabasePath();
    if (path.isEmpty()) {
        m_lastError = QObject::tr(
            "Could not find resources/scripture/bibles.sqlite. Run "
            "scripts/build_bible_db.py to generate it (see "
            "resources/scripture/README.md).");
        return false;
    }
    return openFile(path);
}

bool BibleLibrary::openFile(const QString &sqlitePath)
{
    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_db.setDatabaseName(sqlitePath);
    // Scripture data is bundled read-only; opening read-only avoids
    // accidentally corrupting it and lets multiple app instances share it.
    m_db.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));

    if (!m_db.open()) {
        m_lastError = m_db.lastError().text();
        return false;
    }
    m_lastError.clear();
    m_dbPath = QDir::cleanPath(sqlitePath);
    return true;
}

bool BibleLibrary::isOpen() const
{
    return m_db.isValid() && m_db.isOpen();
}

QVector<BibleLibrary::Translation> BibleLibrary::translations() const
{
    QVector<Translation> result;
    if (!isOpen())
        return result;

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("SELECT code, name FROM translations ORDER BY rowid"));
    if (!query.exec())
        return result;

    while (query.next())
        result.append({query.value(0).toString(), query.value(1).toString()});
    return result;
}

QStringList BibleLibrary::bookNames(const QString &translationCode) const
{
    QStringList result;
    if (!isOpen())
        return result;

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "SELECT name FROM books WHERE translation_code = ? ORDER BY book_index"));
    query.addBindValue(translationCode);
    if (!query.exec())
        return result;

    while (query.next())
        result.append(query.value(0).toString());
    return result;
}

int BibleLibrary::oldTestamentBookCount(const QString &translationCode) const
{
    // All translations bundled today follow the standard 39/27 Protestant
    // split. If a future translation ships with a different book count,
    // this just draws the OT/NT divider in the wrong place visually --
    // harmless, and easy to make data-driven later if it comes up.
    Q_UNUSED(translationCode);
    return 39;
}

int BibleLibrary::chapterCount(const QString &translationCode, const QString &bookName) const
{
    if (!isOpen())
        return 0;

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "SELECT MAX(chapter) FROM verses WHERE translation_code = ? AND book_name = ?"));
    query.addBindValue(translationCode);
    query.addBindValue(bookName);
    if (!query.exec() || !query.next())
        return 0;
    return query.value(0).toInt();
}

int BibleLibrary::verseCount(const QString &translationCode, const QString &bookName, int chapter) const
{
    if (!isOpen())
        return 0;

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "SELECT MAX(verse) FROM verses WHERE translation_code = ? AND book_name = ? AND chapter = ?"));
    query.addBindValue(translationCode);
    query.addBindValue(bookName);
    query.addBindValue(chapter);
    if (!query.exec() || !query.next())
        return 0;
    return query.value(0).toInt();
}

bool BibleLibrary::isValidChapter(const QString &translationCode, const QString &bookName, int chapter) const
{
    if (chapter <= 0)
        return false;
    return chapter <= chapterCount(translationCode, bookName);
}

bool BibleLibrary::isValidVerse(const QString &translationCode, const QString &bookName, int chapter,
                                 int verseNum) const
{
    if (verseNum <= 0)
        return false;
    return verseNum <= verseCount(translationCode, bookName, chapter);
}

QStringList BibleLibrary::matchBookNames(const QString &translationCode, const QString &prefix) const
{
    QStringList result;
    const QString needle = canonicalizeOrdinalPrefix(prefix);
    if (needle.isEmpty())
        return result;

    const QStringList names = bookNames(translationCode);

    // 1. Direct prefix match against the book's own (canonically-cased)
    //    name, e.g. "g" -> Genesis, Galatians; "ge" -> Genesis only.
    //    This also naturally handles numbered books: "1" matches every
    //    "1 ..." book once both sides go through canonicalizeOrdinalPrefix.
    for (const QString &name : names) {
        if (canonicalizeOrdinalPrefix(name).startsWith(needle))
            result << name;
    }
    if (!result.isEmpty())
        return result;

    // 2. Fall back to the abbreviation table for shorthand that isn't a
    //    literal prefix of the full name (e.g. "jn" -> John, "rev" ->
    //    Revelation). Kept in canonical book order and de-duplicated,
    //    since several abbreviations can point at the same book.
    QSet<QString> seen;
    for (auto it = abbreviationTable().constBegin(); it != abbreviationTable().constEnd(); ++it) {
        if (!it.key().startsWith(needle))
            continue;
        const QString targetKey = canonicalizeOrdinalPrefix(it.value());
        for (const QString &name : names) {
            if (canonicalizeOrdinalPrefix(name) == targetKey && !seen.contains(name)) {
                seen.insert(name);
                result << name;
            }
        }
    }
    // Keep canonical (book_index) order rather than abbreviation-table
    // iteration order.
    if (result.size() > 1) {
        const QStringList canonicalOrder = names;
        std::sort(result.begin(), result.end(), [&canonicalOrder](const QString &a, const QString &b) {
            return canonicalOrder.indexOf(a) < canonicalOrder.indexOf(b);
        });
    }
    return result;
}

QVector<BibleLibrary::Verse> BibleLibrary::versesInChapter(const QString &translationCode, const QString &bookName,
                                                            int chapter) const
{
    QVector<Verse> result;
    if (!isOpen())
        return result;

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "SELECT verse, text FROM verses WHERE translation_code = ? AND book_name = ? AND chapter = ? "
        "ORDER BY verse"));
    query.addBindValue(translationCode);
    query.addBindValue(bookName);
    query.addBindValue(chapter);
    if (!query.exec())
        return result;

    while (query.next()) {
        Verse v;
        v.book = bookName;
        v.chapter = chapter;
        v.verse = query.value(0).toInt();
        v.text = query.value(1).toString();
        result.append(v);
    }
    return result;
}

bool BibleLibrary::verseText(const QString &translationCode, const QString &bookName, int chapter, int verseNum,
                              QString *outText) const
{
    if (!isOpen())
        return false;

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "SELECT text FROM verses WHERE translation_code = ? AND book_name = ? AND chapter = ? AND verse = ?"));
    query.addBindValue(translationCode);
    query.addBindValue(bookName);
    query.addBindValue(chapter);
    query.addBindValue(verseNum);
    if (!query.exec() || !query.next())
        return false;

    if (outText)
        *outText = query.value(0).toString();
    return true;
}

QString BibleLibrary::normalizeBookNameLookup(const QString &translationCode, const QString &rawBookName) const
{
    const QStringList names = bookNames(translationCode);
    const QString needle = rawBookName.trimmed();
    if (needle.isEmpty())
        return QString();

    const QString needleKey = canonicalizeOrdinalPrefix(needle);

    // 1. Exact match, tolerant of "1 John" vs "I John" style differences.
    for (const QString &name : names) {
        if (canonicalizeOrdinalPrefix(name) == needleKey)
            return name;
    }

    // 2. Abbreviation table -> canonical name -> confirm it exists in
    //    this translation's book list (it always should, for the four
    //    standard-canon translations shipped, but a differently-ordered
    //    or partial translation shouldn't crash the lookup).
    const auto it = abbreviationTable().find(needleKey);
    if (it != abbreviationTable().end()) {
        const QString targetKey = canonicalizeOrdinalPrefix(it.value());
        for (const QString &name : names) {
            if (canonicalizeOrdinalPrefix(name) == targetKey)
                return name;
        }
    }

    // 3. Prefix match as a last resort ("phili" -> "Philippians", or
    //    "revelation" -> "Revelation of John"), only accepted if it's
    //    unambiguous.
    QString prefixMatch;
    int matchCount = 0;
    for (const QString &name : names) {
        if (canonicalizeOrdinalPrefix(name).startsWith(needleKey)) {
            prefixMatch = name;
            ++matchCount;
        }
    }
    if (matchCount == 1)
        return prefixMatch;

    return QString();
}

BibleLibrary::ParsedReference BibleLibrary::parseReference(const QString &translationCode,
                                                            const QString &input) const
{
    ParsedReference result;
    const QString trimmed = input.trimmed();
    if (trimmed.isEmpty())
        return result;

    // Captures an optional leading book-ordinal number (1/2/3/I/II/III),
    // the book name itself, then optionally "chapter[:verse[-verse]]".
    // Examples matched: "John 3:16", "1 John 3:16-18", "Song of Solomon 2",
    // "Ps 23", "gen 1:1".
    static const QRegularExpression pattern(
        QStringLiteral(R"(^\s*((?:[123]|i{1,3})\s+)?([A-Za-z][A-Za-z ]*?)\s*(?:(\d+)\s*(?::\s*(\d+)(?:\s*-\s*(\d+))?)?)?\s*$)"),
        QRegularExpression::CaseInsensitiveOption);

    const QRegularExpressionMatch match = pattern.match(trimmed);
    if (!match.hasMatch())
        return result;

    QString bookPart = (match.captured(1) + match.captured(2)).trimmed();
    if (bookPart.isEmpty())
        return result;

    const QString canonicalBook = normalizeBookNameLookup(translationCode, bookPart);
    if (canonicalBook.isEmpty())
        return result; // doesn't look like a book we know -- treat as a keyword search instead

    result.book = canonicalBook;
    result.chapter = match.captured(3).isEmpty() ? 1 : match.captured(3).toInt();
    if (!match.captured(4).isEmpty()) {
        result.verseStart = match.captured(4).toInt();
        result.verseEnd = match.captured(5).isEmpty() ? result.verseStart : match.captured(5).toInt();
    }
    result.valid = true;
    return result;
}

QVector<BibleLibrary::SearchResult> BibleLibrary::search(const QString &translationCode, const QString &query,
                                                          int limit) const
{
    QVector<SearchResult> result;
    const QString trimmed = query.trimmed();
    if (!isOpen() || trimmed.isEmpty())
        return result;

    // Build an FTS5 MATCH expression that requires every whitespace-
    // separated term to appear (implicit AND), quoting each term so
    // punctuation in the user's input can't be read as FTS syntax.
    QStringList terms;
    for (const QString &term : trimmed.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts)) {
        QString escaped = term;
        escaped.replace(QLatin1Char('"'), QStringLiteral("\"\""));
        terms << QStringLiteral("\"%1\"").arg(escaped);
    }
    if (terms.isEmpty())
        return result;
    const QString matchExpr = terms.join(QLatin1Char(' '));

    QSqlQuery sqlQuery(m_db);
    sqlQuery.prepare(QStringLiteral(
        "SELECT v.book_name, v.chapter, v.verse, v.text "
        "FROM verses_fts f JOIN verses v ON v.rowid = f.rowid "
        "WHERE f.text MATCH ? AND v.translation_code = ? "
        "ORDER BY v.book_index, v.chapter, v.verse LIMIT ?"));
    sqlQuery.addBindValue(matchExpr);
    sqlQuery.addBindValue(translationCode);
    sqlQuery.addBindValue(limit);

    if (!sqlQuery.exec())
        return result;

    while (sqlQuery.next()) {
        SearchResult r;
        r.book = sqlQuery.value(0).toString();
        r.chapter = sqlQuery.value(1).toInt();
        r.verse = sqlQuery.value(2).toInt();
        r.text = sqlQuery.value(3).toString();
        result.append(r);
    }
    return result;
}

bool BibleLibrary::ensureWritableDatabasePath(QString *outError)
{
    if (m_dbPath.isEmpty()) {
        // Nothing open yet (e.g. openDefault() found no bundled database
        // at all) -- start a brand-new one in a per-user writable
        // location so "Add Bible from disk" still works on a totally
        // fresh install.
        const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dir);
        m_dbPath = QDir(dir).filePath(QStringLiteral("bibles.sqlite"));
        return true;
    }

    QFileInfo info(m_dbPath);
    if (info.exists() && info.isWritable())
        return true;
    // Directory needs to be writable too, for sqlite's journal/WAL files.
    if (info.exists() && QFileInfo(info.absolutePath()).isWritable())
        return true;

    // Bundled database lives somewhere read-only (a packaged install
    // under Program Files or /usr/share, typically). Copy it once to a
    // per-user location and use that copy from now on -- the original
    // bundled translations stay put, only the user's added ones live in
    // the copy.
    const QString userDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(userDir);
    const QString userCopy = QDir(userDir).filePath(QStringLiteral("bibles.sqlite"));

    if (!QFileInfo::exists(userCopy)) {
        if (info.exists() && !QFile::copy(m_dbPath, userCopy)) {
            if (outError)
                *outError = QObject::tr("Could not create a writable copy of the Scripture database at %1.")
                                .arg(userCopy);
            return false;
        }
    }
    m_dbPath = userCopy;
    return true;
}

bool BibleLibrary::importTranslationFromJsonFile(const QString &jsonPath, QString *outCode, QString *outError)
{
    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (outError)
            *outError = QObject::tr("Could not open %1.").arg(jsonPath);
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (outError)
            *outError = QObject::tr("%1 is not a valid Bible JSON file (%2).")
                            .arg(jsonPath, parseError.errorString());
        return false;
    }

    const QJsonObject root = doc.object();
    const QString fullName = root.value(QStringLiteral("translation")).toString();
    const QJsonArray books = root.value(QStringLiteral("books")).toArray();
    if (fullName.isEmpty() || books.isEmpty()) {
        if (outError)
            *outError = QObject::tr(
                "%1 doesn't look like a scrollmapper-format Bible JSON file "
                "(missing \"translation\" or \"books\").").arg(jsonPath);
        return false;
    }

    const QString code = fullName.section(QLatin1Char(':'), 0, 0).trimmed();
    const QString name = fullName.contains(QLatin1Char(':'))
        ? fullName.section(QLatin1Char(':'), 1).trimmed()
        : fullName;
    if (code.isEmpty()) {
        if (outError)
            *outError = QObject::tr("Could not determine a translation code from \"%1\".").arg(fullName);
        return false;
    }

    if (!ensureWritableDatabasePath(outError))
        return false;

    const QString connName = QStringLiteral("bible_import_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    bool ok = true;
    {
        QSqlDatabase importDb = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
        importDb.setDatabaseName(m_dbPath);
        if (!importDb.open()) {
            if (outError)
                *outError = importDb.lastError().text();
            QSqlDatabase::removeDatabase(connName);
            return false;
        }

        QSqlQuery schemaQuery(importDb);
        for (const QString &statement : QString::fromUtf8(kBibleSchemaDdl)
                 .split(QLatin1Char(';'), Qt::SkipEmptyParts)) {
            const QString trimmedStatement = statement.trimmed();
            if (!trimmedStatement.isEmpty() && !schemaQuery.exec(trimmedStatement)) {
                if (outError)
                    *outError = schemaQuery.lastError().text();
                ok = false;
                break;
            }
        }

        if (ok) {
            importDb.transaction();

            QSqlQuery del(importDb);
            for (const QString &table : {QStringLiteral("verses"), QStringLiteral("books"), QStringLiteral("translations")}) {
                del.prepare(QStringLiteral("DELETE FROM %1 WHERE %2 = ?")
                                .arg(table, table == QStringLiteral("translations") ? QStringLiteral("code")
                                                                                     : QStringLiteral("translation_code")));
                del.addBindValue(code);
                if (!del.exec()) {
                    if (outError)
                        *outError = del.lastError().text();
                    ok = false;
                    break;
                }
            }

            if (ok) {
                QSqlQuery insTranslation(importDb);
                insTranslation.prepare(QStringLiteral("INSERT INTO translations(code, name) VALUES (?, ?)"));
                insTranslation.addBindValue(code);
                insTranslation.addBindValue(name);
                if (!insTranslation.exec()) {
                    if (outError)
                        *outError = insTranslation.lastError().text();
                    ok = false;
                }
            }

            if (ok) {
                QSqlQuery insBook(importDb);
                insBook.prepare(
                    QStringLiteral("INSERT INTO books(translation_code, book_index, name) VALUES (?, ?, ?)"));
                QSqlQuery insVerse(importDb);
                insVerse.prepare(QStringLiteral(
                    "INSERT INTO verses(translation_code, book_index, book_name, chapter, verse, text) "
                    "VALUES (?, ?, ?, ?, ?, ?)"));

                int bookIndex = 0;
                for (const QJsonValue &bookVal : books) {
                    const QJsonObject bookObj = bookVal.toObject();
                    const QString bookName = bookObj.value(QStringLiteral("name")).toString();

                    insBook.addBindValue(code);
                    insBook.addBindValue(bookIndex);
                    insBook.addBindValue(bookName);
                    if (!insBook.exec()) {
                        if (outError)
                            *outError = insBook.lastError().text();
                        ok = false;
                        break;
                    }

                    const QJsonArray chapters = bookObj.value(QStringLiteral("chapters")).toArray();
                    for (const QJsonValue &chapterVal : chapters) {
                        const QJsonObject chapterObj = chapterVal.toObject();
                        const int chapterNum = chapterObj.value(QStringLiteral("chapter")).toInt();
                        const QJsonArray verses = chapterObj.value(QStringLiteral("verses")).toArray();
                        for (const QJsonValue &verseVal : verses) {
                            const QJsonObject verseObj = verseVal.toObject();
                            insVerse.addBindValue(code);
                            insVerse.addBindValue(bookIndex);
                            insVerse.addBindValue(bookName);
                            insVerse.addBindValue(chapterObj.value(QStringLiteral("chapter")).toInt());
                            insVerse.addBindValue(verseObj.value(QStringLiteral("verse")).toInt());
                            insVerse.addBindValue(verseObj.value(QStringLiteral("text")).toString());
                            if (!insVerse.exec()) {
                                if (outError)
                                    *outError = insVerse.lastError().text();
                                ok = false;
                                break;
                            }
                        }
                        Q_UNUSED(chapterNum);
                        if (!ok)
                            break;
                    }
                    ++bookIndex;
                    if (!ok)
                        break;
                }
            }

            if (ok)
                importDb.commit();
            else
                importDb.rollback();
        }
    }
    QSqlDatabase::removeDatabase(connName);

    if (!ok)
        return false;

    if (outCode)
        *outCode = code;

    // Re-open our read-only connection against the (possibly new) path
    // so the freshly-imported translation shows up immediately.
    return openFile(m_dbPath);
}
