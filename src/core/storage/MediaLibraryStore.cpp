#include "core/storage/MediaLibraryStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace MediaLibraryStore
{

namespace
{
constexpr int kFormatVersion = 1;
constexpr const char *kFormatTag = "SanctifyLive.medialibrary";

QString kindToString(MediaKind kind)
{
    return kind == MediaKind::Video ? QStringLiteral("video") : QStringLiteral("image");
}

MediaKind kindFromString(const QString &value)
{
    return value == QStringLiteral("video") ? MediaKind::Video : MediaKind::Image;
}
} // namespace

QString defaultStorePath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/media_library.json");
}

bool save(const QVector<Entry> &entries, const QString &path, QString *errorMessage)
{
    QJsonArray itemsArray;
    for (const Entry &entry : entries) {
        QJsonObject obj;
        obj["name"] = entry.name;
        obj["path"] = entry.filePath;
        obj["kind"] = kindToString(entry.kind);
        itemsArray.append(obj);
    }

    QJsonObject root;
    root["format"] = kFormatTag;
    root["version"] = kFormatVersion;
    root["items"] = itemsArray;

    // Make sure the containing directory exists even if the caller
    // passed a path outside defaultStorePath() (e.g. in tests).
    QDir().mkpath(QFileInfo(path).absolutePath());

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage)
            *errorMessage = file.errorString();
        return false;
    }

    const QByteArray bytes = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (file.write(bytes) != bytes.size()) {
        if (errorMessage)
            *errorMessage = file.errorString();
        return false;
    }

    return true;
}

bool load(const QString &path, QVector<Entry> *outEntries, QString *errorMessage)
{
    QFile file(path);
    if (!file.exists()) {
        // Nothing saved yet (first run, or every import so far has been
        // a built-in swatch) -- an empty catalog, not a failure.
        outEntries->clear();
        return true;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage)
            *errorMessage = file.errorString();
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (doc.isNull()) {
        if (errorMessage)
            *errorMessage = parseError.errorString();
        return false;
    }

    const QJsonObject root = doc.object();
    if (root.value("format").toString() != kFormatTag) {
        if (errorMessage)
            *errorMessage = QObject::tr("Not a SanctifyLive media library file.");
        return false;
    }

    QVector<Entry> entries;
    const QJsonArray itemsArray = root.value("items").toArray();
    entries.reserve(itemsArray.size());
    for (const QJsonValue &value : itemsArray) {
        const QJsonObject obj = value.toObject();
        const QString name = obj.value("name").toString();
        const QString filePath = obj.value("path").toString();
        if (name.isEmpty() || filePath.isEmpty())
            continue; // malformed row -- skip rather than fail the whole load
        entries.append(Entry(name, filePath, kindFromString(obj.value("kind").toString())));
    }

    *outEntries = entries;
    return true;
}

} // namespace MediaLibraryStore
