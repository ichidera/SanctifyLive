#include "core/song/SongLibrary.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QUuid>

#include "core/song/SongTextFormat.h"

namespace SongLibrary
{

namespace
{
constexpr int kFormatVersion = 1;
constexpr const char *kFormatTag = "SanctifyLive.songlibrary";

// Builds one Song from a title/author and the same bracket-tagged text
// format the Songs tab's editor uses (see SongTextFormat.h) -- so the
// bundled sample hymns below are parsed through the exact same code
// path a hand-typed song would be, rather than hand-built
// QVector<SongSection> literals that could quietly drift out of sync
// with what the parser actually accepts.
Song makeSong(const QString &title, const QString &author, const QString &bracketText,
              const QStringList &verseOrder)
{
    Song song;
    song.id = newSongId();
    song.title = title;
    song.author = author;
    SongTextFormat::parse(bracketText, &song.sections);
    song.verseOrder = verseOrder;
    return song;
}
} // namespace

QString defaultStorePath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/song_library.json");
}

QString newSongId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

bool save(const QVector<Song> &songs, const QString &path, QString *errorMessage)
{
    QJsonArray songsArray;
    for (const Song &song : songs) {
        QJsonArray sectionsArray;
        for (const SongSection &section : song.sections) {
            QJsonObject sectionObj;
            sectionObj["type"] = section.type;
            sectionObj["number"] = section.number;
            sectionObj["text"] = section.text;
            sectionsArray.append(sectionObj);
        }

        QJsonObject songObj;
        songObj["id"] = song.id;
        songObj["title"] = song.title;
        songObj["author"] = song.author;
        songObj["sections"] = sectionsArray;
        songObj["verseOrder"] = QJsonArray::fromStringList(song.verseOrder);
        songsArray.append(songObj);
    }

    QJsonObject root;
    root["format"] = kFormatTag;
    root["version"] = kFormatVersion;
    root["songs"] = songsArray;

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

bool load(const QString &path, QVector<Song> *outSongs, QString *errorMessage)
{
    QFile file(path);
    if (!file.exists()) {
        // Nothing saved yet (first run) -- an empty catalog, not a failure.
        outSongs->clear();
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
            *errorMessage = QObject::tr("Not a SanctifyLive song library file.");
        return false;
    }

    QVector<Song> songs;
    const QJsonArray songsArray = root.value("songs").toArray();
    songs.reserve(songsArray.size());
    for (const QJsonValue &songValue : songsArray) {
        const QJsonObject songObj = songValue.toObject();
        const QString title = songObj.value("title").toString();
        if (title.isEmpty())
            continue; // malformed row -- skip rather than fail the whole load

        Song song;
        song.id = songObj.value("id").toString();
        if (song.id.isEmpty())
            song.id = newSongId(); // tolerate an old/hand-edited file missing an id rather than losing the song
        song.title = title;
        song.author = songObj.value("author").toString();

        const QJsonArray sectionsArray = songObj.value("sections").toArray();
        song.sections.reserve(sectionsArray.size());
        for (const QJsonValue &sectionValue : sectionsArray) {
            const QJsonObject sectionObj = sectionValue.toObject();
            SongSection section;
            section.type = sectionObj.value("type").toString();
            section.number = sectionObj.value("number").toInt(1);
            section.text = sectionObj.value("text").toString();
            if (section.type.isEmpty())
                continue;
            song.sections.append(section);
        }

        for (const QJsonValue &tagValue : songObj.value("verseOrder").toArray())
            song.verseOrder.append(tagValue.toString());

        songs.append(song);
    }

    *outSongs = songs;
    return true;
}

QVector<Song> sampleSongs()
{
    // Four hymns, all well over a century old and long in the public
    // domain in the United States (author death dates and/or original
    // publication dates make this unambiguous regardless of which PD
    // test is applied) -- bundled in full so the Songs tab has real
    // content on first run, the same reasoning as the Scriptures tab
    // shipping real Bible text rather than an empty placeholder. Unlike
    // NIV/HCSB (see core/scripture/ScriptureLibrary.h), there is no
    // publisher licence question here: nobody holds a copyright on
    // these words to license in the first place.
    QVector<Song> songs;

    songs.append(makeSong(
        QStringLiteral("Amazing Grace"), QStringLiteral("John Newton, 1779"),
        QStringLiteral("[Verse 1]\n"
                        "Amazing grace! How sweet the sound\n"
                        "That saved a wretch like me!\n"
                        "I once was lost, but now am found;\n"
                        "Was blind, but now I see.\n"
                        "\n"
                        "[Verse 2]\n"
                        "'Twas grace that taught my heart to fear,\n"
                        "And grace my fears relieved;\n"
                        "How precious did that grace appear\n"
                        "The hour I first believed.\n"
                        "\n"
                        "[Verse 3]\n"
                        "Through many dangers, toils, and snares,\n"
                        "I have already come;\n"
                        "'Tis grace hath brought me safe thus far,\n"
                        "And grace will lead me home.\n"
                        "\n"
                        "[Verse 4]\n"
                        "When we've been there ten thousand years,\n"
                        "Bright shining as the sun,\n"
                        "We've no less days to sing God's praise\n"
                        "Than when we first begun."),
        {"v1", "v2", "v3", "v4"}));

    songs.append(makeSong(
        QStringLiteral("Holy, Holy, Holy"), QStringLiteral("Reginald Heber, 1826"),
        QStringLiteral("[Verse 1]\n"
                        "Holy, holy, holy! Lord God Almighty!\n"
                        "Early in the morning our song shall rise to Thee.\n"
                        "Holy, holy, holy, merciful and mighty!\n"
                        "God in three Persons, blessed Trinity!\n"
                        "\n"
                        "[Verse 2]\n"
                        "Holy, holy, holy! All the saints adore Thee,\n"
                        "Casting down their golden crowns around the glassy sea;\n"
                        "Cherubim and seraphim falling down before Thee,\n"
                        "Which wert, and art, and evermore shalt be.\n"
                        "\n"
                        "[Verse 3]\n"
                        "Holy, holy, holy! Though the darkness hide Thee,\n"
                        "Though the eye of sinful man Thy glory may not see,\n"
                        "Only Thou art holy; there is none beside Thee,\n"
                        "Perfect in power, in love, and purity.\n"
                        "\n"
                        "[Verse 4]\n"
                        "Holy, holy, holy! Lord God Almighty!\n"
                        "All Thy works shall praise Thy name, in earth, and sky, and sea.\n"
                        "Holy, holy, holy, merciful and mighty!\n"
                        "God in three Persons, blessed Trinity!"),
        {"v1", "v2", "v3", "v4"}));

    songs.append(makeSong(
        QStringLiteral("It Is Well with My Soul"), QStringLiteral("Horatio Spafford, 1873"),
        QStringLiteral("[Verse 1]\n"
                        "When peace, like a river, attendeth my way,\n"
                        "When sorrows like sea billows roll;\n"
                        "Whatever my lot, Thou hast taught me to say,\n"
                        "It is well, it is well, with my soul.\n"
                        "\n"
                        "[Chorus]\n"
                        "It is well with my soul,\n"
                        "It is well, it is well with my soul.\n"
                        "\n"
                        "[Verse 2]\n"
                        "Though Satan should buffet, though trials should come,\n"
                        "Let this blest assurance control,\n"
                        "That Christ has regarded my helpless estate,\n"
                        "And hath shed His own blood for my soul.\n"
                        "\n"
                        "[Verse 3]\n"
                        "My sin, oh, the bliss of this glorious thought,\n"
                        "My sin, not in part but the whole,\n"
                        "Is nailed to the cross, and I bear it no more:\n"
                        "Praise the Lord, praise the Lord, O my soul!"),
        {"v1", "c1", "v2", "c1", "v3", "c1"}));

    songs.append(makeSong(
        QStringLiteral("A Mighty Fortress Is Our God"),
        QStringLiteral("Martin Luther, 1529; tr. Frederic H. Hedge, 1852"),
        QStringLiteral("[Verse 1]\n"
                        "A mighty fortress is our God,\n"
                        "A bulwark never failing;\n"
                        "Our helper He, amid the flood\n"
                        "Of mortal ills prevailing.\n"
                        "For still our ancient foe\n"
                        "Doth seek to work us woe;\n"
                        "His craft and power are great,\n"
                        "And, armed with cruel hate,\n"
                        "On earth is not his equal.\n"
                        "\n"
                        "[Verse 2]\n"
                        "Did we in our own strength confide,\n"
                        "Our striving would be losing,\n"
                        "Were not the right Man on our side,\n"
                        "The Man of God's own choosing.\n"
                        "Dost ask who that may be?\n"
                        "Christ Jesus, it is He;\n"
                        "Lord Sabaoth, His name,\n"
                        "From age to age the same,\n"
                        "And He must win the battle."),
        {"v1", "v2"}));

    return songs;
}

} // namespace SongLibrary
