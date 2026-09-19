#include "core/song/Song.h"

#include <QCoreApplication>

QString SongSection::tag() const
{
    return type.toLower() + QString::number(number);
}

QString SongSection::displayLabel() const
{
    const QString t = type.toLower();
    QString typeName;
    if (t == QLatin1String("v"))
        typeName = QCoreApplication::translate("SongSection", "Verse");
    else if (t == QLatin1String("c"))
        typeName = QCoreApplication::translate("SongSection", "Chorus");
    else if (t == QLatin1String("b"))
        typeName = QCoreApplication::translate("SongSection", "Bridge");
    else if (t == QLatin1String("p"))
        typeName = QCoreApplication::translate("SongSection", "Pre-Chorus");
    else if (t == QLatin1String("i"))
        typeName = QCoreApplication::translate("SongSection", "Intro");
    else if (t == QLatin1String("e"))
        typeName = QCoreApplication::translate("SongSection", "Ending");
    else
        typeName = QCoreApplication::translate("SongSection", "Other");
    return QStringLiteral("%1 %2").arg(typeName).arg(number);
}

QVector<const SongSection *> Song::orderedSections() const
{
    QVector<const SongSection *> result;

    if (verseOrder.isEmpty()) {
        result.reserve(sections.size());
        for (const SongSection &section : sections)
            result.append(&section);
        return result;
    }

    result.reserve(verseOrder.size());
    for (const QString &wantedTag : verseOrder) {
        for (const SongSection &section : sections) {
            if (QString::compare(section.tag(), wantedTag, Qt::CaseInsensitive) == 0) {
                result.append(&section);
                break;
            }
        }
    }
    return result;
}
