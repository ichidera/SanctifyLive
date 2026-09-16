#include "core/scripture/ScriptureLibrary.h"

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

namespace ScriptureLibrary
{

namespace
{
constexpr const char *kResourcePath = ":/scriptures/KJV.json";
constexpr const char *kTranslationCode = "KJV";

QVector<ScriptureVerse> loadFromResource()
{
    QVector<ScriptureVerse> result;

    QFile file(kResourcePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "ScriptureLibrary: couldn't open bundled resource" << kResourcePath;
        return result;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (doc.isNull()) {
        qWarning() << "ScriptureLibrary: failed to parse" << kResourcePath << ":" << parseError.errorString();
        return result;
    }

    const QJsonArray books = doc.object().value("books").toArray();
    result.reserve(31200); // KJV has 31,102 verses; a little headroom avoids a mid-load reallocation.

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

} // namespace

QString translationCode()
{
    return QString::fromLatin1(kTranslationCode);
}

const QVector<ScriptureVerse> &verses()
{
    // Loaded once, on first use, and cached for the life of the
    // process -- 31,000+ verses is cheap to hold in memory permanently
    // but not something every ScripturePanel construction (or a future
    // second window) should re-parse from JSON.
    static const QVector<ScriptureVerse> cached = loadFromResource();
    return cached;
}

const QVector<ScriptureBookInfo> &books()
{
    // Derived from verses() in a single linear pass -- verses() is
    // already in canonical book/chapter order, so each book's
    // chapter-count and per-chapter verse-count fall out just by
    // noticing when the book or chapter name changes.
    static const QVector<ScriptureBookInfo> cached = [] {
        QVector<ScriptureBookInfo> result;
        const QVector<ScriptureVerse> &all = verses();
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
        return result;
    }();
    return cached;
}

int rowForReference(const QString &book, int chapter, int verse)
{
    for (const ScriptureBookInfo &info : books()) {
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
