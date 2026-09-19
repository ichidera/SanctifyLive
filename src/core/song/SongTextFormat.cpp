#include "core/song/SongTextFormat.h"

#include <QMap>
#include <QRegularExpression>

namespace SongTextFormat
{

namespace
{

QString typeLetterFor(const QString &word)
{
    const QString w = QString(word).toLower().remove(QLatin1Char('-'));
    if (w == QLatin1String("verse"))
        return QStringLiteral("v");
    if (w == QLatin1String("chorus"))
        return QStringLiteral("c");
    if (w == QLatin1String("bridge"))
        return QStringLiteral("b");
    if (w == QLatin1String("prechorus"))
        return QStringLiteral("p");
    if (w == QLatin1String("intro"))
        return QStringLiteral("i");
    if (w == QLatin1String("ending"))
        return QStringLiteral("e");
    return QStringLiteral("o");
}

QString headerWordFor(const QString &typeLetter)
{
    const QString t = typeLetter.toLower();
    if (t == QLatin1String("v"))
        return QStringLiteral("Verse");
    if (t == QLatin1String("c"))
        return QStringLiteral("Chorus");
    if (t == QLatin1String("b"))
        return QStringLiteral("Bridge");
    if (t == QLatin1String("p"))
        return QStringLiteral("Pre-Chorus");
    if (t == QLatin1String("i"))
        return QStringLiteral("Intro");
    if (t == QLatin1String("e"))
        return QStringLiteral("Ending");
    return QStringLiteral("Other");
}

} // namespace

bool parse(const QString &text, QVector<SongSection> *outSections)
{
    outSections->clear();

    static const QRegularExpression tagPattern(
        QStringLiteral(R"(^\s*\[\s*(verse|chorus|bridge|pre-?chorus|intro|ending|other)\s*(\d+)?\s*\]\s*$)"),
        QRegularExpression::CaseInsensitiveOption);

    // Tracks "how many un-numbered sections of this type have been
    // auto-numbered so far", independent of any explicitly-numbered
    // sections of the same type -- so "[Verse]" "[Verse]" "[Verse]"
    // (no numbers at all) predictably becomes Verse 1/2/3 regardless of
    // whatever explicit numbers appear elsewhere in the text.
    QMap<QString, int> nextImplicitNumber;

    SongSection current;
    QStringList currentLines;
    bool haveCurrent = false;

    auto finalizeCurrent = [&]() {
        if (!haveCurrent)
            return;
        while (!currentLines.isEmpty() && currentLines.last().trimmed().isEmpty())
            currentLines.removeLast(); // trim trailing blank lines; internal blank lines (stanza breaks) are kept
        current.text = currentLines.join(QStringLiteral("\n"));
        outSections->append(current);
        currentLines.clear();
    };

    const QStringList lines = text.split(QLatin1Char('\n'));
    for (const QString &rawLine : lines) {
        const QRegularExpressionMatch match = tagPattern.match(rawLine);
        if (match.hasMatch()) {
            finalizeCurrent();
            const QString typeLetter = typeLetterFor(match.captured(1));
            int number;
            if (!match.captured(2).isEmpty()) {
                number = match.captured(2).toInt();
            } else {
                number = nextImplicitNumber.value(typeLetter, 0) + 1;
                nextImplicitNumber[typeLetter] = number;
            }
            current = SongSection();
            current.type = typeLetter;
            current.number = number;
            haveCurrent = true;
        } else if (haveCurrent) {
            currentLines.append(rawLine);
        }
        // Lines before the first tag are silently dropped, as documented.
    }
    finalizeCurrent();

    return !outSections->isEmpty();
}

QString render(const QVector<SongSection> &sections)
{
    QStringList blocks;
    blocks.reserve(sections.size());
    for (const SongSection &section : sections) {
        const QString header = QStringLiteral("%1 %2").arg(headerWordFor(section.type)).arg(section.number);
        blocks << QStringLiteral("[%1]\n%2").arg(header, section.text);
    }
    return blocks.join(QStringLiteral("\n\n"));
}

} // namespace SongTextFormat
