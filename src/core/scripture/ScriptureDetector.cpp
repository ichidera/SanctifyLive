#include "core/scripture/ScriptureDetector.h"

#include <algorithm>

#include <QRegularExpression>
#include <QStringList>

#include "core/scripture/ScriptureLibrary.h"
#include "core/scripture/ScriptureReference.h"

namespace ScriptureDetector
{

namespace
{

// Matches any recognized book-name variant, as a whole word,
// case-insensitively. Built once from ScriptureReference::
// allBookNameVariants() rather than hand-listed, so it can never drift
// out of sync with the book/alias list the structured search box uses.
const QRegularExpression &bookNamePattern()
{
    static const QRegularExpression pattern = [] {
        QVector<ScriptureReference::BookNameVariant> variants = ScriptureReference::allBookNameVariants();
        // Longest display text first, so the alternation prefers the
        // more specific match if one variant's text is ever a prefix of
        // another's -- defensive; no such collision exists in the
        // current book list, but this keeps that from becoming a silent
        // bug if one is ever added.
        std::sort(variants.begin(), variants.end(),
                  [](const auto &a, const auto &b) { return a.displayText.size() > b.displayText.size(); });

        QStringList escaped;
        escaped.reserve(variants.size());
        for (const auto &variant : variants)
            escaped << QRegularExpression::escape(variant.displayText);

        return QRegularExpression(QStringLiteral(R"(\b(%1)\b)").arg(escaped.join(QLatin1Char('|'))),
                                   QRegularExpression::CaseInsensitiveOption);
    }();
    return pattern;
}

QString canonicalNameFor(const QString &matchedDisplayText)
{
    for (const auto &variant : ScriptureReference::allBookNameVariants()) {
        if (QString::compare(variant.displayText, matchedDisplayText, Qt::CaseInsensitive) == 0)
            return variant.canonicalName;
    }
    return QString();
}

// Matches the chapter/verse portion immediately following a book-name
// mention: an optional "chapter" word, the chapter number, an optional
// ":" / "," / "verse(s)" separator and the verse number, and an
// optional "-" or "to" plus an end verse. Anchored to the START of
// whatever's passed in (the caller only ever passes the text right
// after a book-name match), not to a full line, since it's meant to
// describe "what comes right after the book name", not to search on
// its own.
const QRegularExpression &chapterVersePattern()
{
    static const QRegularExpression pattern(
        QStringLiteral(
            R"(^[\s,]*(?:chapter\s+)?(\d{1,3})[\s,:]*(?:verses?\s*)?(\d{1,3})(?:\s*(?:-|to)\s*(\d{1,3}))?)"),
        QRegularExpression::CaseInsensitiveOption);
    return pattern;
}

} // namespace

QVector<Detection> scan(const QString &translationCode, const QString &text)
{
    QVector<Detection> results;

    int searchFrom = 0;
    while (searchFrom < text.size()) {
        const QRegularExpressionMatch bookMatch = bookNamePattern().match(text, searchFrom);
        if (!bookMatch.hasMatch())
            break;

        const QString canonicalBook = canonicalNameFor(bookMatch.captured(1));
        const int afterBookName = bookMatch.capturedEnd();
        if (canonicalBook.isEmpty()) {
            searchFrom = afterBookName; // shouldn't happen, but skip past it rather than loop forever
            continue;
        }

        // A short trailing window is enough for even a verbose spoken
        // phrasing like "chapter 8, verses 28 to 30" -- not scanning the
        // whole rest of the text keeps a false book-name match (a name
        // mentioned in conversation, with numbers appearing much later
        // for unrelated reasons) from accidentally pairing with them.
        const QString remainder = text.mid(afterBookName, 40);
        const QRegularExpressionMatch cvMatch = chapterVersePattern().match(remainder);
        if (!cvMatch.hasMatch()) {
            searchFrom = afterBookName;
            continue;
        }

        const int chapter = cvMatch.captured(1).toInt();
        const int startVerse = cvMatch.captured(2).toInt();
        const int matchEnd = afterBookName + cvMatch.capturedEnd();

        const int firstRow = ScriptureLibrary::rowForReference(translationCode, canonicalBook, chapter, startVerse);
        if (firstRow < 0) {
            // Reference-shaped but not a real verse in this translation
            // (chapter/verse number too high, etc) -- skip rather than
            // report a broken detection.
            searchFrom = afterBookName;
            continue;
        }

        int lastRow = firstRow;
        if (!cvMatch.captured(3).isEmpty()) {
            const int endVerse = cvMatch.captured(3).toInt();
            const int candidateLastRow =
                ScriptureLibrary::rowForReference(translationCode, canonicalBook, chapter, endVerse);
            if (candidateLastRow >= firstRow)
                lastRow = candidateLastRow;
        }

        const QVector<ScriptureVerse> &verses = ScriptureLibrary::verses(translationCode);
        QStringList textParts;
        for (int row = firstRow; row <= lastRow && row < verses.size(); ++row)
            textParts << verses.at(row).text;

        Detection detection;
        detection.position = bookMatch.capturedStart();
        detection.matchedPhrase = text.mid(detection.position, matchEnd - detection.position);
        detection.reference = lastRow > firstRow
            ? QStringLiteral("%1-%2").arg(verses.at(firstRow).reference).arg(verses.at(lastRow).verse)
            : verses.at(firstRow).reference;
        detection.text = textParts.join(QLatin1Char(' '));
        results.append(detection);

        searchFrom = matchEnd;
    }

    return results;
}

} // namespace ScriptureDetector
