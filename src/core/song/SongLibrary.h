#pragma once

#include <QString>
#include <QVector>

#include "core/song/Song.h"

// SongLibrary is the on-disk memory for the Songs tab's catalog (see
// ui/SongsPanel) -- mirrors core/storage/MediaLibraryStore's shape
// deliberately (same defaultStorePath()/save()/load() signatures, same
// "missing file is an empty catalog, not an error" rule) so anyone
// already familiar with how the Media tab persists its catalog already
// knows how this one works.
//
// Unlike MediaLibraryStore, which deliberately does NOT copy the real
// media files it references, songs' actual content (the lyrics
// themselves) IS what gets stored here -- there's no external file to
// point at instead, the way there is for a photo or video.
//
// Kept in core/ (no Qt widgets) so it can be unit tested or reused
// headlessly, same rationale as ScheduleIO/MediaLibraryStore.
namespace SongLibrary
{

// Where the catalog lives: <AppDataLocation>/song_library.json. Creates
// the containing directory if it doesn't exist yet.
QString defaultStorePath();

// A fresh, never-before-used song id (a UUID). Call once when creating
// a new Song; existing songs keep whatever id they were given.
QString newSongId();

// Serializes `songs` to a simple JSON document at `path`. Returns true
// on success; on failure `errorMessage` (if non-null) is set to a
// human-readable reason.
bool save(const QVector<Song> &songs, const QString &path, QString *errorMessage = nullptr);

// Reads a catalog previously written by save(). Returns true and fills
// `outSongs` on success. A missing file (first run) is treated as
// success with an empty list, not an error. Genuine parse/format
// failures still report false via `errorMessage`.
bool load(const QString &path, QVector<Song> *outSongs, QString *errorMessage = nullptr);

// A handful of public-domain hymns (see the .cpp for exactly which, and
// why each is safe to bundle in full) so the Songs tab has real,
// working content to show and search on first run, the same way the
// Scriptures tab ships with real Bible text rather than starting empty.
// Callers decide whether/when to seed a fresh library with these (see
// ui/SongsPanel) -- this function itself has no side effects.
QVector<Song> sampleSongs();

} // namespace SongLibrary
