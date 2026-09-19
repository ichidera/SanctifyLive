#include "core/scripture/ScriptureLibrary.h"

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMap>

namespace ScriptureLibrary
{

namespace
{

QVector<ScriptureVerse> loadFromResource(const QString &resourcePath)
{
    QVector<ScriptureVerse> result;

    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "ScriptureLibrary: couldn't open bundled resource" << resourcePath;
        return result;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (doc.isNull()) {
        qWarning() << "ScriptureLibrary: failed to parse" << resourcePath << ":" << parseError.errorString();
        return result;
    }

    const QJsonArray books = doc.object().value("books").toArray();
    result.reserve(31200); // every bundled translation has ~31,100 verses; a little headroom avoids a mid-load reallocation

    for (const QJsonValue &bookValue : books) {
        const QJsonObject bookObj = bookValue.toObject();
        const QString bookName = bookObj.value("name").toString();
        if (bookName.isEmpty())
            continue; // malformed entry -- skip it rather than fail the whole library

        const QJsonArray chapters = bookObj.value("chapters").toArray();
        for (const QJsonValue &chapterValue : chapters) {
            const QJsonObject chapterObj = chapterValue.toObject();
            const int chapterNumber = chapterObj.value("chapter").toInt();

            const QJsonArray verseArray = chapterObj.value("verses").toArray();
            for (const QJsonValue &verseValue : verseArray) {
                const QJsonObject verseObj = verseValue.toObject();
                const int verseNumber = verseObj.value("verse").toInt();
                const QString text = verseObj.value("text").toString();
                if (text.isEmpty())
                    continue; // malformed entry -- skip it, same reasoning as above

                ScriptureVerse verse;
                verse.book = bookName;
                verse.chapter = chapterNumber;
                verse.verse = verseNumber;
                verse.text = text;
                verse.reference = QStringLiteral("%1 %2:%3").arg(bookName).arg(chapterNumber).arg(verseNumber);
                result.append(verse);
            }
        }
    }

    return result;
}

QString resourcePathFor(const QString &translationCode)
{
    for (const ScriptureTranslation &t : availableTranslations()) {
        if (t.code == translationCode)
            return QStringLiteral(":/scriptures/%1.json").arg(translationCode);
    }
    return QString(); // not a bundled translation
}

// Node-based (not contiguous-storage) associative container: inserting
// a new key never moves or invalidates the QVector already stored under
// a different key, which is what makes it safe for verses()/books()
// below to hand back long-lived references into these caches rather
// than copying a translation's full text on every call.
QMap<QString, QVector<ScriptureVerse>> &verseCache()
{
    static QMap<QString, QVector<ScriptureVerse>> cache;
    return cache;
}

QMap<QString, QVector<ScriptureBookInfo>> &bookInfoCache()
{
    static QMap<QString, QVector<ScriptureBookInfo>> cache;
    return cache;
}

} // namespace

const QVector<ScriptureTranslation> &availableTranslations()
{
    // See this header's file-level comment for why exactly these four,
    // and specifically why NIV/HCSB are not here.
    static const QVector<ScriptureTranslation> table = {
        {QStringLiteral("KJV"), QStringLiteral("King James Version (1769)"), QStringLiteral("en")},
        {QStringLiteral("ASV"), QStringLiteral("American Standard Version (1901)"), QStringLiteral("en")},
        {QStringLiteral("NHEB"), QStringLiteral("New Heart English Bible"), QStringLiteral("en")},
        {QStringLiteral("RVA"), QStringLiteral("Reina-Valera Antigua (1909)"), QStringLiteral("es")},
    };
    return table;
}

const QVector<ScriptureVerse> &verses(const QString &translationCode)
{
    QMap<QString, QVector<ScriptureVerse>> &cache = verseCache();
    auto it = cache.find(translationCode);
    if (it == cache.end()) {
        const QString path = resourcePathFor(translationCode);
        if (path.isEmpty())
            qWarning() << "ScriptureLibrary: unknown translation code" << translationCode;
        // Loaded once, on first use, and cached for the life of the
        // process -- 31,000+ verses is cheap to hold in memory
        // permanently but not something every ScripturePanel
        // construction (or switching translations back and forth)
        // should re-parse from JSON. Translations never selected are
        // never loaded at all.
        it = cache.insert(translationCode, path.isEmpty() ? QVector<ScriptureVerse>() : loadFromResource(path));
    }
    return it.value();
}

const QVector<ScriptureBookInfo> &books(const QString &translationCode)
{
    QMap<QString, QVector<ScriptureBookInfo>> &cache = bookInfoCache();
    auto it = cache.find(translationCode);
    if (it == cache.end()) {
        // Derived from verses() in a single linear pass -- verses() is
        // already in canonical book/chapter order, so each book's
        // chapter-count and per-chapter verse-count fall out just by
        // noticing when the book or chapter name changes.
        QVector<ScriptureBookInfo> result;
        const QVector<ScriptureVerse> &all = verses(translationCode);
        for (int i = 0; i < all.size(); ++i) {
            const ScriptureVerse &v = all.at(i);
            if (result.isEmpty() || result.last().name != v.book) {
                ScriptureBookInfo info;
                info.name = v.book;
                info.firstVerseRow = i;
                result.append(info);
            }
            ScriptureBookInfo &current = result.last();
            if (v.chapter > current.versesPerChapter.size())
                current.versesPerChapter.resize(v.chapter, 0);
            // Verses arrive in increasing order within a chapter, so the
            // last one written for a given chapter is that chapter's
            // true verse count.
            current.versesPerChapter[v.chapter - 1] = v.verse;
        }
        it = cache.insert(translationCode, result);
    }
    return it.value();
}

int rowForReference(const QString &translationCode, const QString &book, int chapter, int verse)
{
    for (const ScriptureBookInfo &info : books(translationCode)) {
        if (QString::compare(info.name, book, Qt::CaseInsensitive) != 0)
            continue;
        if (chapter < 1 || chapter > info.chapterCount())
            return -1;
        if (verse < 1 || verse > info.versesPerChapter.at(chapter - 1))
            return -1;
        int offset = 0;
        for (int c = 0; c < chapter - 1; ++c)
            offset += info.versesPerChapter.at(c);
        return info.firstVerseRow + offset + (verse - 1);
    }
    return -1;
}

} // namespace ScriptureLibrary
