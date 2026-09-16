#include "core/scripture/ScriptureReference.h"

#include <algorithm>

#include <QCoreApplication>
#include <QPair>
#include <QRegularExpression>

#include "core/scripture/ScriptureLibrary.h"

namespace ScriptureReference
{

namespace
{

// Common alternate spellings for the numbered books: KJV.json stores
// them as "I Samuel"/"II Kings"/etc, but most people type "1
// Samuel"/"2 Kings". Only the LEADING numeral is rewritten, so this is
// safe to apply to the whole book-query text.
QString normalizeNumberedPrefix(const QString &text)
{
    static const QVector<QPair<QString, QString>> kPrefixAliases = {
        {QStringLiteral("1 "), QStringLiteral("I ")},
        {QStringLiteral("2 "), QStringLiteral("II ")},
        {QStringLiteral("3 "), QStringLiteral("III ")},
    };
    for (const auto &pair : kPrefixAliases) {
        if (text.startsWith(pair.first, Qt::CaseInsensitive))
            return pair.second + text.mid(pair.first.size());
    }
    return text;
}

// "Revelation" is how almost everyone refers to the book KJV.json
// literally calls "Revelation of John" -- worth a direct alias rather
// than making the operator guess the data's exact wording.
QString applyFriendlyAlias(const QString &text)
{
    if (QString::compare(text, QStringLiteral("Revelation"), Qt::CaseInsensitive) == 0)
        return QStringLiteral("Revelation of John");
    return text;
}

struct BookMatch
{
    QString name;
    bool exact = false;
};

// All books whose (alias-normalized) name equals or starts with
// `rawQuery`, in canonical Bible order -- canonical order both because
// it's a sensible display order for suggestions and because it makes
// "exactly one match" checks stable.
QVector<BookMatch> matchBooks(const QString &rawQuery)
{
    QVector<BookMatch> matches;
    const QString query = applyFriendlyAlias(normalizeNumberedPrefix(rawQuery.trimmed()));
    if (query.isEmpty())
        return matches;

    for (const ScriptureBookInfo &info : ScriptureLibrary::books()) {
        if (QString::compare(info.name, query, Qt::CaseInsensitive) == 0)
            matches.append({info.name, true});
        else if (info.name.startsWith(query, Qt::CaseInsensitive))
            matches.append({info.name, false});
    }
    return matches;
}

const ScriptureBookInfo *findBookInfo(const QString &canonicalName)
{
    for (const ScriptureBookInfo &info : ScriptureLibrary::books()) {
        if (info.name == canonicalName)
            return &info;
    }
    return nullptr;
}

// Matches (in order):
//   1: book text -- an optional "1 "/"2 "/"3 " numeral prefix, then
//      letters/spaces/periods. Digits are structurally excluded from
//      this group, so it can never accidentally swallow a chapter or
//      verse number.
//   2: chapter digits (optional)
//   3: verse digits (optional; only meaningful if 2 is present)
//   4: end-verse digits (optional; only meaningful if 3 is present)
const QRegularExpression &referencePattern()
{
    static const QRegularExpression pattern(
        QStringLiteral(R"(^\s*((?:[123]\s+)?[A-Za-z][A-Za-z .]*)\s*)"
                        R"((?:(\d+)\s*(?::\s*(\d+)\s*(?:[-\x{2013}]\s*(\d+))?)?)?\s*$)"));
    return pattern;
}

} // namespace

Parsed parse(const QString &input)
{
    Parsed result;

    const QRegularExpressionMatch match = referencePattern().match(input);
    if (!match.hasMatch()) {
        // Doesn't even look like "letters, then optionally numbers" --
        // e.g. starts with a bare digit. Nothing resolves; report the
        // raw text back as the book query so the UI can still say
        // "that's not a book" rather than nothing at all.
        result.bookQuery = input.trimmed();
        return result;
    }

    result.bookQuery = match.captured(1).trimmed();
    const QString chapterText = match.captured(2);
    const QString verseText = match.captured(3);
    const QString endVerseText = match.captured(4);

    const QVector<BookMatch> bookMatches = matchBooks(result.bookQuery);
    QString resolvedBook;
    if (bookMatches.size() == 1) {
        resolvedBook = bookMatches.first().name;
    } else if (bookMatches.size() > 1) {
        const int exactCount =
            int(std::count_if(bookMatches.begin(), bookMatches.end(), [](const BookMatch &m) { return m.exact; }));
        if (exactCount == 1) {
            for (const BookMatch &m : bookMatches) {
                if (m.exact) {
                    resolvedBook = m.name;
                    break;
                }
            }
        } else {
            // Genuinely ambiguous ("J" -> Joshua/Judges/John/Job). If the
            // operator has already typed a chapter number, they've moved
            // past picking a book, but we still can't tell which one they
            // meant -- surface that rather than silently guessing.
            result.bookAmbiguous = true;
        }
    }

    if (resolvedBook.isEmpty()) {
        result.stage = Stage::Book;
        return result;
    }
    result.book = resolvedBook;

    const ScriptureBookInfo *info = findBookInfo(resolvedBook);
    if (!info) {
        result.stage = Stage::Book; // shouldn't happen, but degrade safely
        return result;
    }

    if (chapterText.isEmpty()) {
        result.stage = Stage::Chapter;
        return result;
    }

    const int chapter = chapterText.toInt();
    if (chapter < 1 || chapter > info->chapterCount()) {
        result.stage = Stage::Chapter;
        result.error = QCoreApplication::translate("ScriptureReference", "%1 has %2 chapter(s).")
                           .arg(resolvedBook)
                           .arg(info->chapterCount());
        return result;
    }
    result.chapter = chapter;

    const int maxVerse = info->versesPerChapter.at(chapter - 1);

    if (verseText.isEmpty()) {
        result.stage = Stage::Verse;
        return result;
    }

    const int verse = verseText.toInt();
    if (verse < 1 || verse > maxVerse) {
        result.stage = Stage::Verse;
        result.error = QCoreApplication::translate("ScriptureReference", "%1 %2 has %3 verse(s).")
                           .arg(resolvedBook)
                           .arg(chapter)
                           .arg(maxVerse);
        return result;
    }
    result.startVerse = verse;
    result.stage = Stage::Range;
    result.firstRow = ScriptureLibrary::rowForReference(resolvedBook, chapter, verse);
    result.lastRow = result.firstRow;
    result.valid = result.firstRow >= 0;

    if (!endVerseText.isEmpty()) {
        const int endVerse = endVerseText.toInt();
        if (endVerse < verse || endVerse > maxVerse) {
            // The range end is bad, but the start verse the operator
            // already typed is still a real verse -- keep `valid` true
            // for that rather than throwing away a working reference
            // just because of what comes after the dash.
            result.error = QCoreApplication::translate("ScriptureReference", "%1 %2 has %3 verse(s).")
                               .arg(resolvedBook)
                               .arg(chapter)
                               .arg(maxVerse);
        } else {
            result.endVerse = endVerse;
            result.lastRow = ScriptureLibrary::rowForReference(resolvedBook, chapter, endVerse);
        }
    }

    return result;
}

QVector<BookSuggestion> bookSuggestions(const QString &bookQuery, int maxResults)
{
    QVector<BookSuggestion> result;
    if (bookQuery.trimmed().isEmpty())
        return result;

    for (const BookMatch &match : matchBooks(bookQuery)) {
        if (result.size() >= maxResults)
            break;
        result.append({match.name, match.name + QStringLiteral(" ")});
    }
    return result;
}

} // namespace ScriptureReference
