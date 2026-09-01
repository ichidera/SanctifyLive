#include "TranslationLibraryDialog.h"

#include <QDesktopServices>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTemporaryFile>
#include <QUrl>
#include <QVBoxLayout>

#include "BibleLibrary.h"

namespace {
constexpr int kColTranslation = 0;
constexpr int kColAbbreviation = 1;
constexpr int kColLanguage = 2;
constexpr int kColAvailable = 3;

// Raw JSON URL -> repository page, so "View source" / manual download
// always has somewhere sensible to land even without the dialog's own
// downloader.
QString repoPageForCode(const QString &code)
{
    return QStringLiteral("https://github.com/scrollmapper/bible_databases/blob/master/formats/json/%1.json")
        .arg(code);
}
}

QVector<TranslationLibraryDialog::CatalogEntry> TranslationLibraryDialog::catalog()
{
    // A modest, verified-to-exist slice of scrollmapper/bible_databases'
    // formats/json directory -- all public-domain, all free. This isn't
    // meant to be exhaustive; it's meant to be real and working. The
    // repository has more (and the "Get" flow works with any translation
    // code that lives at .../formats/json/<CODE>.json), so this list can
    // grow over time without touching the download/import code at all.
    static const QVector<CatalogEntry> entries = {
        {QStringLiteral("AKJV"), QStringLiteral("American King James Version"), QStringLiteral("English"),
         QStringLiteral("https://raw.githubusercontent.com/scrollmapper/bible_databases/master/formats/json/AKJV.json")},
        {QStringLiteral("DRC"), QStringLiteral("Douay-Rheims 1899 American Edition"), QStringLiteral("English"),
         QStringLiteral("https://raw.githubusercontent.com/scrollmapper/bible_databases/master/formats/json/DRC.json")},
        {QStringLiteral("LEB"), QStringLiteral("Lexham English Bible"), QStringLiteral("English"),
         QStringLiteral("https://raw.githubusercontent.com/scrollmapper/bible_databases/master/formats/json/LEB.json")},
        {QStringLiteral("NHEB"), QStringLiteral("New Heart English Bible"), QStringLiteral("English"),
         QStringLiteral("https://raw.githubusercontent.com/scrollmapper/bible_databases/master/formats/json/NHEB.json")},
        {QStringLiteral("CPDV"), QStringLiteral("Catholic Public Domain Version"), QStringLiteral("English"),
         QStringLiteral("https://raw.githubusercontent.com/scrollmapper/bible_databases/master/formats/json/CPDV.json")},
        {QStringLiteral("WLC"), QStringLiteral("Westminster Leningrad Codex"), QStringLiteral("Hebrew"),
         QStringLiteral("https://raw.githubusercontent.com/scrollmapper/bible_databases/master/formats/json/WLC.json")},
    };
    return entries;
}

TranslationLibraryDialog::TranslationLibraryDialog(BibleLibrary *library, const QStringList &installedCodes,
                                                     QWidget *parent)
    : QDialog(parent), m_library(library)
{
    setWindowTitle(tr("More Available"));
    resize(620, 380);

    auto *intro = new QLabel(
        tr("Every translation below is free -- sourced from the open, public-domain "
           "scrollmapper/bible_databases repository. Pick one to download and add it to your library."),
        this);
    intro->setWordWrap(true);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({tr("Translation"), tr("Abbreviation"), tr("Language"), tr("Available")});
    m_table->horizontalHeader()->setSectionResizeMode(kColTranslation, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(kColAbbreviation, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(kColLanguage, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(kColAvailable, QHeaderView::ResizeToContents);
    m_table->verticalHeader()->setVisible(false);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet(QStringLiteral("color: #888;"));
    m_statusLabel->setWordWrap(true);

    auto *diskButton = new QPushButton(tr("Add Bible from Disk..."), this);
    diskButton->setToolTip(
        tr("Import a scrollmapper-format Bible JSON file you already have saved locally."));
    connect(diskButton, &QPushButton::clicked, this, &TranslationLibraryDialog::handleAddFromDiskClicked);

    auto *closeButton = new QPushButton(tr("Close"), this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    auto *bottomRow = new QHBoxLayout();
    bottomRow->addWidget(diskButton);
    bottomRow->addStretch(1);
    bottomRow->addWidget(closeButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(intro);
    layout->addWidget(m_table, 1);
    layout->addWidget(m_statusLabel);
    layout->addLayout(bottomRow);

    m_network = new QNetworkAccessManager(this);

    populateTable(installedCodes);
}

void TranslationLibraryDialog::populateTable(const QStringList &installedCodes)
{
    m_rows = catalog();
    m_table->setRowCount(m_rows.size());

    for (int row = 0; row < m_rows.size(); ++row) {
        const CatalogEntry &entry = m_rows.at(row);
        m_table->setItem(row, kColTranslation, new QTableWidgetItem(entry.name));
        m_table->setItem(row, kColAbbreviation, new QTableWidgetItem(entry.code));
        m_table->setItem(row, kColLanguage, new QTableWidgetItem(entry.language));
        m_table->item(row, kColTranslation)->setToolTip(repoPageForCode(entry.code));

        const bool alreadyInstalled = installedCodes.contains(entry.code, Qt::CaseInsensitive);
        if (alreadyInstalled) {
            auto *label = new QLabel(tr("Installed"), m_table);
            label->setStyleSheet(QStringLiteral("color: #6fbf73; padding-left: 6px;"));
            m_table->setCellWidget(row, kColAvailable, label);
            continue;
        }

        auto *cell = new QWidget(m_table);
        auto *cellLayout = new QHBoxLayout(cell);
        cellLayout->setContentsMargins(4, 2, 4, 2);
        auto *freeLabel = new QLabel(tr("Free"), cell);
        freeLabel->setStyleSheet(QStringLiteral("color: #6fbf73;"));
        auto *getButton = new QPushButton(tr("Get"), cell);
        getButton->setFixedWidth(56);
        cellLayout->addWidget(freeLabel);
        cellLayout->addWidget(getButton);
        m_table->setCellWidget(row, kColAvailable, cell);

        connect(getButton, &QPushButton::clicked, this, [this, row]() { handleGetClicked(row); });
    }
}

void TranslationLibraryDialog::setRowBusy(int row, bool busy)
{
    if (auto *cell = m_table->cellWidget(row, kColAvailable)) {
        if (auto *button = cell->findChild<QPushButton *>())
            button->setEnabled(!busy);
    }
}

void TranslationLibraryDialog::handleGetClicked(int row)
{
    if (row < 0 || row >= m_rows.size())
        return;
    const CatalogEntry entry = m_rows.at(row);

    setRowBusy(row, true);
    m_statusLabel->setText(tr("Downloading %1...").arg(entry.name));

    QNetworkReply *reply = m_network->get(QNetworkRequest(QUrl(entry.jsonUrl)));
    connect(reply, &QNetworkReply::finished, this, [this, reply, row, entry]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            setRowBusy(row, false);
            m_statusLabel->setText(tr("Couldn't download %1: %2. You can also open the "
                                       "repository page directly.")
                                        .arg(entry.name, reply->errorString()));
            return;
        }

        QTemporaryFile tempFile(this);
        tempFile.setAutoRemove(false);
        if (!tempFile.open()) {
            setRowBusy(row, false);
            m_statusLabel->setText(tr("Couldn't create a temporary file to import %1.").arg(entry.name));
            return;
        }
        tempFile.write(reply->readAll());
        const QString tempPath = tempFile.fileName();
        tempFile.close();

        importDownloadedJson(row, tempPath, entry.code);
    });
}

void TranslationLibraryDialog::importDownloadedJson(int row, const QString &tempFilePath, const QString &expectedCode)
{
    QString importedCode, error;
    const bool ok = m_library->importTranslationFromJsonFile(tempFilePath, &importedCode, &error);
    QFile::remove(tempFilePath);

    if (!ok) {
        setRowBusy(row, false);
        m_statusLabel->setText(tr("Couldn't add %1: %2").arg(expectedCode, error));
        return;
    }

    m_statusLabel->setText(tr("%1 added -- it's ready to use.").arg(importedCode));
    auto *doneLabel = new QLabel(tr("Installed"), m_table);
    doneLabel->setStyleSheet(QStringLiteral("color: #6fbf73; padding-left: 6px;"));
    m_table->setCellWidget(row, kColAvailable, doneLabel);

    emit translationAdded(importedCode);
}

void TranslationLibraryDialog::handleAddFromDiskClicked()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Add Bible from Disk"), QString(),
        tr("Bible JSON files (*.json);;All files (*)"));
    if (path.isEmpty())
        return;

    QString code, error;
    if (!m_library->importTranslationFromJsonFile(path, &code, &error)) {
        QMessageBox::warning(this, tr("Couldn't Add Bible"), error);
        return;
    }

    m_statusLabel->setText(tr("%1 added from disk -- it's ready to use.").arg(code));
    emit translationAdded(code);
}
