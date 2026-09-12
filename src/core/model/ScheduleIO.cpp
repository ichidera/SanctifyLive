#include "core/model/ScheduleIO.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace ScheduleIO
{

namespace
{
constexpr int kFormatVersion = 1;
}

bool saveToFile(const QVector<Slide> &slides, const QString &path, QString *errorMessage)
{
    QJsonArray slidesArray;
    for (const Slide &slide : slides) {
        QJsonObject obj;
        obj["label"] = slide.label;
        obj["text"] = slide.text;
        obj["background"] = slide.background.name(QColor::HexArgb);
        slidesArray.append(obj);
    }

    QJsonObject root;
    root["format"] = "SanctifyLive.schedule";
    root["version"] = kFormatVersion;
    root["slides"] = slidesArray;

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

bool loadFromFile(const QString &path, QVector<Slide> *outSlides, QString *errorMessage)
{
    QFile file(path);
    if (!file.exists()) {
        if (errorMessage)
            *errorMessage = QObject::tr("File does not exist: %1").arg(QFileInfo(path).fileName());
        return false;
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
    if (root.value("format").toString() != "SanctifyLive.schedule") {
        if (errorMessage)
            *errorMessage = QObject::tr("Not a SanctifyLive schedule file.");
        return false;
    }

    QVector<Slide> slides;
    const QJsonArray slidesArray = root.value("slides").toArray();
    slides.reserve(slidesArray.size());
    for (const QJsonValue &value : slidesArray) {
        const QJsonObject obj = value.toObject();
        QColor background = QColor(obj.value("background").toString());
        if (!background.isValid())
            background = Qt::black;
        slides.append(Slide(obj.value("label").toString(),
                             obj.value("text").toString(),
                             background));
    }

    *outSlides = slides;
    return true;
}

} // namespace ScheduleIO
