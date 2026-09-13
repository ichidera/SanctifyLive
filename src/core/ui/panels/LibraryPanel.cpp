#include "core/ui/panels/LibraryPanel.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPixmap>
#include <QSplitter>
#include <QTabWidget>
#include <QVBoxLayout>

namespace
{
// The set of media categories shown in the design mockup. Each one maps
// to a real subfolder under the media root -- dropping files into that
// folder is what makes them show up here.
const QStringList kMediaCategories = {
    QStringLiteral("Videos"),  QStringLiteral("Images"), QStringLiteral("Feeds"),
    QStringLiteral("DVD"),     QStringLiteral("Time"),   QStringLiteral("Audio"),
    QStringLiteral("Collections"),
};

const QStringList kImageExtensions = {"*.png", "*.jpg", "*.jpeg", "*.bmp", "*.gif", "*.webp"};
} // namespace

LibraryPanel::LibraryPanel(QWidget *parent) : QWidget(parent)
{
    // Kept beside the executable rather than in a hidden app-data folder
    // so it's obvious to an operator where to drop files -- this is a
    // volunteer-run tool, not something with an import wizard yet.
    m_mediaRoot = QCoreApplication::applicationDirPath() + "/media";
    for (const QString &category : kMediaCategories) {
        QDir().mkpath(m_mediaRoot + "/" + category);
    }

    auto *tabs = new QTabWidget(this);
    tabs->addTab(buildMediaTab(), tr("Media"));
    tabs->addTab(buildPlaceholderTab(tr(
                     "Song lyrics library isn't built yet -- see the README "
                     "roadmap. Slides are added manually for now.")),
                 tr("Songs"));
    tabs->addTab(buildPlaceholderTab(tr(
                     "Scripture lookup isn't built yet -- see the README "
                     "roadmap. Verses are added manually for now.")),
                 tr("Scriptures"));
    tabs->addTab(buildPlaceholderTab(tr(
                     "Saved presentations aren't built yet -- there's no "
                     "file format to load/save from disk yet.")),
                 tr("Presentations"));
    tabs->addTab(buildPlaceholderTab(tr(
                     "Theme presets (background/font bundles) aren't built "
                     "yet -- each slide's background is set individually.")),
                 tr("Themes"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(tabs);
}

QWidget *LibraryPanel::buildMediaTab()
{
    m_mediaFolders = new QListWidget(this);
    for (const QString &category : kMediaCategories) {
        m_mediaFolders->addItem(category);
    }
    connect(m_mediaFolders, &QListWidget::currentItemChanged,
            this, &LibraryPanel::onMediaFolderChanged);

    m_mediaFiles = new QListWidget(this);
    m_mediaFiles->setViewMode(QListView::IconMode);
    m_mediaFiles->setIconSize(QSize(96, 72));
    m_mediaFiles->setResizeMode(QListView::Adjust);
    m_mediaFiles->setMovement(QListView::Static);
    m_mediaFiles->setWordWrap(true);
    m_mediaFiles->setSpacing(8);

    auto *splitter = new QSplitter(this);
    splitter->addWidget(m_mediaFolders);
    splitter->addWidget(m_mediaFiles);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({140, 480});

    auto *container = new QWidget(this);
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->addWidget(splitter);

    m_mediaFolders->setCurrentRow(1); // default to "Images" like the mockup
    return container;
}

QWidget *LibraryPanel::buildPlaceholderTab(const QString &description)
{
    auto *label = new QLabel(description, this);
    label->setProperty("role", "muted");
    label->setWordWrap(true);
    label->setAlignment(Qt::AlignCenter);

    auto *container = new QWidget(this);
    auto *layout = new QVBoxLayout(container);
    layout->addStretch(1);
    layout->addWidget(label);
    layout->addStretch(1);
    return container;
}

void LibraryPanel::onMediaFolderChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    Q_UNUSED(previous);
    if (!current)
        return;
    populateMediaFiles(current->text());
}

void LibraryPanel::populateMediaFiles(const QString &categoryName)
{
    m_mediaFiles->clear();

    const QString folderPath = m_mediaRoot + "/" + categoryName;
    const bool isImageFolder = (categoryName == QStringLiteral("Images"));

    QDir dir(folderPath);
    const QStringList nameFilters = isImageFolder ? kImageExtensions : QStringList{"*"};
    const QFileInfoList entries =
        dir.entryInfoList(nameFilters, QDir::Files, QDir::Name);

    if (entries.isEmpty()) {
        auto *hint = new QListWidgetItem(
            tr("(empty -- drop files into)\n%1").arg(folderPath));
        hint->setFlags(Qt::NoItemFlags); // informational only, not selectable
        m_mediaFiles->addItem(hint);
        return;
    }

    QFileIconProvider iconProvider;
    for (const QFileInfo &info : entries) {
        QIcon icon;
        if (isImageFolder) {
            const QPixmap pixmap(info.absoluteFilePath());
            icon = pixmap.isNull() ? iconProvider.icon(info)
                                    : QIcon(pixmap.scaled(96, 72, Qt::KeepAspectRatio,
                                                           Qt::SmoothTransformation));
        } else {
            icon = iconProvider.icon(info);
        }

        auto *item = new QListWidgetItem(icon, info.fileName());
        item->setData(Qt::UserRole, info.absoluteFilePath());
        m_mediaFiles->addItem(item);
    }
}
