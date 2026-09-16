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

} // namespace ScriptureLibrary
