#pragma once

#include <QString>
#include <QVector>

// MediaLibraryStore is the on-disk memory for the Media tab's *imported*
// catalog (see ui/MediaLibraryPanel): the list of real image/video files
// an operator has clicked "+" on. Without this, every relaunch of the
// app started that catalog over at empty -- the original files were
// still safely on disk, but the app had no idea it had ever seen them,
// so an operator had to re-import their whole media set every service.
//
// Deliberately NOT copying media files into app-owned storage: these are
// often large video files living on a church's shared drive or USB
// stick, and silently duplicating gigabytes of video without being
// asked is the wrong default for a volunteer-run tool. Instead this
// just remembers the (name, kind, path) triples the operator pointed
// at, the same way ScheduleIO remembers Slide::backgroundImagePath
// rather than embedding pixels. If a remembered file later goes missing
// (moved, deleted, USB stick unplugged), the caller's job is to notice
// that at load time and drop the entry rather than show a broken tile
// -- see MediaLibraryPanel::MediaLibraryPanel().
//
// Kept in core/ (no Qt widgets, no rendering) so it can be unit tested
// or reused headlessly, same rationale as core/model/ScheduleIO.
namespace MediaLibraryStore
{

enum class MediaKind
{
    Image,
    Video,
};

struct Entry
{
    QString name;     // display name, e.g. "Beach Sunset" -- shown in the tile
    QString filePath; // absolute path to the real file on disk
    MediaKind kind = MediaKind::Image;

    Entry() = default;
    Entry(QString name_, QString filePath_, MediaKind kind_)
        : name(std::move(name_)), filePath(std::move(filePath_)), kind(kind_)
    {
    }
};

// Where the catalog lives: <AppDataLocation>/media_library.json. A
// per-user app-data path (not "next to the executable") because unlike
// media files themselves, this manifest is small, private bookkeeping
// that has no reason to be something an operator drags around on a USB
// stick or checks into a service folder. Creates the containing
// directory if it doesn't exist yet.
QString defaultStorePath();

// Serializes `entries` to a simple JSON document at `path`. Returns
// true on success; on failure `errorMessage` (if non-null) is set to a
// human-readable reason.
bool save(const QVector<Entry> &entries, const QString &path, QString *errorMessage = nullptr);

// Reads a catalog previously written by save(). Returns true and fills
// `outEntries` on success. A missing file (first run, nothing imported
// yet) is treated as success with an empty list, not an error --
// callers shouldn't have to special-case "never saved before".
// Genuine parse/format failures still report false via `errorMessage`.
bool load(const QString &path, QVector<Entry> *outEntries, QString *errorMessage = nullptr);

} // namespace MediaLibraryStore
