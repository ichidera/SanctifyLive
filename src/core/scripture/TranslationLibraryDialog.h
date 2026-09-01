#ifndef SANCTIFYLIVE_CORE_SCRIPTURE_TRANSLATIONLIBRARYDIALOG_H_
#define SANCTIFYLIVE_CORE_SCRIPTURE_TRANSLATIONLIBRARYDIALOG_H_

#include <QDialog>
#include <QVector>

class BibleLibrary;
class QLabel;
class QNetworkAccessManager;
class QNetworkReply;
class QPushButton;
class QTableWidget;

// TranslationLibraryDialog is the "More Available..." screen. Every
// entry it lists is a public-domain / freely-licensed translation
// pulled from the scrollmapper/bible_databases repository (the same
// source scripts/build_bible_db.py already builds the bundled database
// from) -- there is no paid tier here, so every row simply reads "Free"
// and "Get" downloads + imports it directly into the app.
//
// The bottom of the dialog also offers "Add Bible from disk...", for a
// translation the person already has as a local scrollmapper-format
// JSON file (or a full replacement bibles.sqlite) -- this is the same
// entry point exposed from ScripturePanel's "+" button, kept here too
// since it's the natural place to look for it.
class TranslationLibraryDialog : public QDialog
{
    Q_OBJECT

public:
    // `library` is not owned; it must outlive the dialog. `installedCodes`
    // is used purely to grey out / mark translations already present so
    // the person doesn't try to "Get" something they already have.
    TranslationLibraryDialog(BibleLibrary *library, const QStringList &installedCodes, QWidget *parent = nullptr);

signals:
    // Emitted once for every translation successfully added (whether via
    // download or from-disk import), so the panel can refresh its
    // translation list and, on the first one, switch to it.
    void translationAdded(const QString &code);

private slots:
    void handleGetClicked(int row);
    void handleAddFromDiskClicked();

private:
    struct CatalogEntry
    {
        QString code;
        QString name;
        QString language;
        QString jsonUrl;
    };

    static QVector<CatalogEntry> catalog();
    void populateTable(const QStringList &installedCodes);
    void setRowBusy(int row, bool busy);
    void importDownloadedJson(int row, const QString &tempFilePath, const QString &expectedCode);

    BibleLibrary *m_library;
    QTableWidget *m_table;
    QLabel *m_statusLabel;
    QNetworkAccessManager *m_network;
    QVector<CatalogEntry> m_rows;
};

#endif // SANCTIFYLIVE_CORE_SCRIPTURE_TRANSLATIONLIBRARYDIALOG_H_
