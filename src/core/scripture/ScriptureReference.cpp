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

// Book names are the same 66-book-canon identifiers across every
// bundled translation (see ScriptureLibrary.h) -- "Genesis" is
// "Genesis" whether the verse text under it is in English or Spanish --
// so book matching doesn't need a translation code. KJV is used as an
// arbitrary representative to enumerate the canonical name list; any
// bundled translation would give the same book names.
const QVector<ScriptureBookInfo> &canonicalBookList()
{
    return ScriptureLibrary::books(QStringLiteral("KJV"));
}

// Common alternate spellings for the numbered books: the data stores
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

// "Revelation" is how almost everyone refers to the book the data
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

    for (const ScriptureBookInfo &info : canonicalBookList()) {
        if (QString::compare(info.name, query, Qt::CaseInsensitive) == 0)
            matches.append({info.name, true});
        else if (info.name.startsWith(query, Qt::CaseInsensitive))
            matches.append({info.name, false});
    }
    return matches;
}

const ScriptureBookInfo *findBookInfo(const QString &translationCode, const QString &canonicalName)
{
    for (const ScriptureBookInfo &info : ScriptureLibrary::books(translationCode)) {
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
//   3: start-verse digits (optional; only meaningful if 2 is present).
//      The separator before it is `[:\s]+` -- colon OR whitespace --
//      matching both "John 3:16" (typed reference) and "John 3 16"
//      (EasyWorship-style Spacebar-separated Quick Search entry).
//   4: end digits after a dash (optional; only meaningful if 3 is
//      present). Ambiguous on its own between "end verse in the same
//      chapter" and "end chapter of a cross-chapter range" -- group 5
//      disambiguates.
//   5: end-verse digits after a second colon/space (optional; only
//      present for a cross-chapter range like "John 3:16-4:2", in which
//      case group 4 is actually the END chapter and group 5 is the end
//      verse within it).
const QRegularExpression &referencePattern()
{
    static const QRegularExpression pattern(QStringLiteral(
        R"(^\s*((?:[123]\s+)?[A-Za-z][A-Za-z .]*)\s*)"
        R"((?:(\d+)(?:[:\s]+(\d+)(?:\s*[-\x{2013}]\s*(\d+)(?:[:\s]+(\d+))?)?)?)?\s*$)"));
    return pattern;
}

} // namespace

Parsed parse(const QString &translationCode, const QString &input)
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
    const QString startVerseText = match.captured(3);
    const QString dashNumberText = match.captured(4); // end verse OR end chapter, see below
    const QString crossChapterEndVerseText = match.captured(5);

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

    const ScriptureBookInfo *info = findBookInfo(translationCode, resolvedBook);
    if (!info) {
        // Shouldn't happen for any of the app's real translation codes
        // (every bundled translation covers the same 66 books) -- but
        // degrade safely and consistently: report this the same way as
        // "no book resolved yet" rather than leaving `book` set while
        // `stage` says otherwise.
        result.book.clear();
        result.stage = Stage::Book;
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

    const int maxVerseInChapter = info->versesPerChapter.at(chapter - 1);

    if (startVerseText.isEmpty()) {
        result.stage = Stage::Verse;
        return result;
    }

    const int startVerse = startVerseText.toInt();
    if (startVerse < 1 || startVerse > maxVerseInChapter) {
        result.stage = Stage::Verse;
        result.error = QCoreApplication::translate("ScriptureReference", "%1 %2 has %3 verse(s).")
                           .arg(resolvedBook)
                           .arg(chapter)
                           .arg(maxVerseInChapter);
        return result;
    }
    result.startVerse = startVerse;
    result.stage = Stage::Range;
    result.firstRow = ScriptureLibrary::rowForReference(translationCode, resolvedBook, chapter, startVerse);
    result.lastRow = result.firstRow;
    result.valid = result.firstRow >= 0;

    if (dashNumberText.isEmpty())
        return result; // no range requested -- single verse is still a fully valid result

    if (!crossChapterEndVerseText.isEmpty()) {
        // "John 3:16-4:2" -- dashNumberText is the END CHAPTER, and
        // crossChapterEndVerseText is the end verse within it.
        const int endChapter = dashNumberText.toInt();
        const int endVerse = crossChapterEndVerseText.toInt();
        const bool chapterInRange = endChapter >= chapter && endChapter <= info->chapterCount();
        const int maxVerseInEndChapter = chapterInRange ? info->versesPerChapter.at(endChapter - 1) : 0;
        const bool sameChapterOrderOk = endChapter > chapter || endVerse >= startVerse;

        if (!chapterInRange || endVerse < 1 || endVerse > maxVerseInEndChapter || !sameChapterOrderOk) {
            // The range end is bad, but the start verse the operator
            // already typed is still a real verse -- keep `valid` true
            // for that rather than throwing away a working reference
            // just because of what comes after the dash.
            result.error = QCoreApplication::translate("ScriptureReference", "%1 has %2 chapter(s).")
                               .arg(resolvedBook)
                               .arg(info->chapterCount());
        } else {
            result.endChapter = endChapter;
            result.endVerse = endVerse;
            result.lastRow = ScriptureLibrary::rowForReference(translationCode, resolvedBook, endChapter, endVerse);
        }
        return result;
    }

    // "John 3:16-18" -- dashNumberText is an end verse within the SAME chapter.
    const int endVerse = dashNumberText.toInt();
    if (endVerse < startVerse || endVerse > maxVerseInChapter) {
        result.error = QCoreApplication::translate("ScriptureReference", "%1 %2 has %3 verse(s).")
                           .arg(resolvedBook)
                           .arg(chapter)
                           .arg(maxVerseInChapter);
    } else {
        result.endChapter = chapter;
        result.endVerse = endVerse;
        result.lastRow = ScriptureLibrary::rowForReference(translationCode, resolvedBook, chapter, endVerse);
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

QVector<BookNameVariant> allBookNameVariants()
{
    QVector<BookNameVariant> result;
    for (const ScriptureBookInfo &info : canonicalBookList()) {
        result.append({info.name, info.name}); // every book matches its own canonical name, trivially

        // Numbered-book alias, reversed from normalizeNumberedPrefix():
        // "I Samuel" -> also recognize "1 Samuel" and the spoken form
        // "First Samuel" (a sermon is spoken, not typed -- "First John"
        // is at least as common there as "1 John", and mis-resolving it
        // to the Gospel of John instead of I John would show the
        // congregation the wrong verse entirely, not just miss a
        // detection).
        static const QVector<QPair<QString, QString>> kRomanToArabic = {
            {QStringLiteral("I "), QStringLiteral("1 ")},
            {QStringLiteral("II "), QStringLiteral("2 ")},
            {QStringLiteral("III "), QStringLiteral("3 ")},
        };
        static const QVector<QPair<QString, QString>> kRomanToSpokenWord = {
            {QStringLiteral("I "), QStringLiteral("First ")},
            {QStringLiteral("II "), QStringLiteral("Second ")},
            {QStringLiteral("III "), QStringLiteral("Third ")},
        };
        for (const auto &pair : kRomanToArabic) {
            if (info.name.startsWith(pair.first)) {
                result.append({pair.second + info.name.mid(pair.first.size()), info.name});
                break;
            }
        }
        for (const auto &pair : kRomanToSpokenWord) {
            if (info.name.startsWith(pair.first)) {
                result.append({pair.second + info.name.mid(pair.first.size()), info.name});
                break;
            }
        }

        // "Revelation" is how almost everyone actually says/writes it.
        if (info.name == QLatin1String("Revelation of John"))
            result.append({QStringLiteral("Revelation"), info.name});
    }
    return result;
}

} // namespace ScriptureReference
