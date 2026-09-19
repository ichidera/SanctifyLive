#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

// One labeled section of a song's lyrics -- a verse, chorus, bridge,
// etc -- stored once regardless of how many times it repeats in the
// song's actual running order (see Song::verseOrder). This mirrors the
// verse-tagging scheme used by OpenLyrics, the de facto interchange
// format most song-based worship-presentation tools (OpenLP included)
// use to move songs between programs -- a song entered here speaks the
// same vocabulary a file exported from one of those would, even though
// SanctifyLive doesn't import/export OpenLyrics files itself yet.
struct SongSection
{
    // Single-letter section type, OpenLyrics-style: "v" verse, "c"
    // chorus, "b" bridge, "p" pre-chorus, "i" intro, "e" ending.
    // Anything else typed by the operator is kept as-is and displayed
    // via displayLabel()'s "Other" fallback rather than rejected.
    QString type;
    int number = 1; // "v2" -> type="v", number=2
    QString text;   // this section's full lyrics, possibly multiple lines

    // The tag this section is referenced by in Song::verseOrder, e.g.
    // "v2", "c1" -- lowercase type + number, no space. Two sections
    // sharing a tag would be ambiguous, so SongLibrary/SongTextFormat
    // treat tags as unique within a song.
    QString tag() const;

    // Human-readable label for UI, e.g. "Verse 2", "Chorus 1".
    QString displayLabel() const;
};

struct Song
{
    QString id; // stable, unique -- generated once when the song is created (see SongLibrary::newSongId()), never reused
    QString title;
    QString author; // optional, may be empty

    // Every distinct section, in whatever order they were entered. This
    // is the song's "vocabulary" -- verseOrder (below) is what actually
    // controls projection order, and may repeat any tag here any number
    // of times (a chorus sung after every verse is stored ONCE here,
    // referenced repeatedly in verseOrder).
    QVector<SongSection> sections;

    // Space-separated section tags defining projection order, e.g.
    // {"v1","c1","v2","c1","v3","c1"}. Empty means "no explicit order
    // was given" -- see orderedSections(), which falls back to
    // `sections` in storage order in that case.
    QStringList verseOrder;

    // The sections to actually project, in order: verseOrder resolved
    // against `sections` (falling back to `sections`' own storage order
    // if verseOrder is empty). A tag in verseOrder that doesn't match
    // any stored section is skipped rather than treated as an error --
    // a song with a typo'd tag should still project what it can rather
    // than refuse to display at all.
    QVector<const SongSection *> orderedSections() const;
};
