#include "MediaLibraryPanel.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QSplitter>
#include <QTabWidget>
#include <QToolButton>
#include <QTreeWidget>

#include "../common/IconFactory.h"
#include "ImageFramingDialog.h"
#include "../scripture/ScripturePanel.h"

namespace {
constexpr int kGridIconSize = 64;
constexpr int kPreviewSize = 200;

QPixmap makeSwatch(const QColor &color, int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    QLinearGradient gradient(0, 0, size, size);
    gradient.setColorAt(0, color.lighter(125));
    gradient.setColorAt(1, color.darker(115));

    painter.setPen(QPen(color.darker(160), 1));
    painter.setBrush(gradient);
    painter.drawRoundedRect(QRectF(0.5, 0.5, size - 1, size - 1), 4, 4);
    return pixmap;
}

// Cover-fit crop of a real imported image into a square thumbnail, with
// the same rounded-corner treatment as the placeholder swatches so the
// grid looks consistent regardless of where an entry came from. Honors
// the same focus point the live output crops toward (see
// MediaEntry::focus / Slide::backgroundFocus) so the thumbnail always
// shows what will actually end up on screen, not just whatever a
// dead-center crop happened to keep.
QPixmap makeImageThumbnail(const QString &path, int size, const QPointF &focus)
{
    QPixmap source(path);
    if (source.isNull())
        return makeSwatch(QColor("#555555"), size);

    const QPixmap scaled = source.scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    const int maxX = qMax(0, scaled.width() - size);
    const int maxY = qMax(0, scaled.height() - size);
    const int x = qBound(0, qRound(focus.x() * scaled.width() - size / 2.0), maxX);
    const int y = qBound(0, qRound(focus.y() * scaled.height() - size / 2.0), maxY);
    const QPixmap cropped = scaled.copy(QRect(x, y, size, size));

    QPixmap rounded(size, size);
    rounded.fill(Qt::transparent);
    QPainter painter(&rounded);
    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath clip;
    clip.addRoundedRect(QRectF(0.5, 0.5, size - 1, size - 1), 4, 4);
    painter.setClipPath(clip);
    painter.drawPixmap(0, 0, cropped);
    return rounded;
}

// A category header row ("MEDIA", "COLLECTIONS") -- visible for
// grouping, but not itself a selectable/clickable category.
QTreeWidgetItem *makeSectionHeader(QTreeWidget *tree, const QString &text)
{
    auto *item = new QTreeWidgetItem(tree, {text});
    QFont font = item->font(0);
    font.setBold(true);
    font.setPointSizeF(font.pointSizeF() * 0.9);
    item->setFont(0, font);
    item->setForeground(0, QColor("#8a8a8a"));
    item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
    return item;
}

QTreeWidgetItem *makeCategory(QTreeWidgetItem *parent, const QString &text, const QIcon &icon)
{
    auto *item = new QTreeWidgetItem(parent, {text});
    item->setIcon(0, icon);
    return item;
}

// Categories that plausibly mean "a file on disk" for import purposes.
// Feeds/DVD don't map to a simple file picker, so import falls back to
// a generic "any file" dialog for those rather than being hidden --
// consistent with the request that the "+" import control show up the
// same way in every category.
QString fileFilterForCategory(const QString &category)
{
    if (category == QObject::tr("Images"))
        return QObject::tr("Images (*.png *.jpg *.jpeg *.bmp *.gif);;All Files (*)");
    if (category == QObject::tr("Videos"))
        return QObject::tr("Videos (*.mp4 *.mov *.avi *.mkv *.webm);;All Files (*)");
    if (category == QObject::tr("Audio"))
        return QObject::tr("Audio (*.mp3 *.wav *.ogg *.flac);;All Files (*)");
    return QObject::tr("All Files (*)");
}

// A stable, arbitrary color derived from the filename, purely so
// imported non-image files (video/audio/etc., which we can't thumbnail
// yet) get visually distinct placeholder swatches instead of all
// looking identical.
QColor colorFromName(const QString &name)
{
    const uint hash = qHash(name);
    return QColor::fromHsv(hash % 360, 130, 190);
}
}

MediaLibraryPanel::MediaLibraryPanel(QWidget *parent) : QWidget(parent)
{
    populateSampleMedia();

    // ---------- Content-type tab row ----------
    m_contentTabs = new QTabWidget(this);

    m_contentTabs->addTab(new QWidget(this), tr("Songs"));

    m_scripturePanel = new ScripturePanel(this);
    connect(m_scripturePanel, &ScripturePanel::scriptureActivated, this, &MediaLibraryPanel::scriptureActivated);
    const int scripturesTabIndex = m_contentTabs->addTab(m_scripturePanel, tr("Scriptures"));

    // --- Media tab (the only functional one for now) ---
    auto *mediaTab = new QWidget(this);

    m_searchBox = new QLineEdit(mediaTab);
    m_searchBox->setPlaceholderText(tr("Search Any Field"));
    m_searchBox->addAction(IconFactory::search(14), QLineEdit::LeadingPosition);

    m_categoryTree = new QTreeWidget(mediaTab);
    m_categoryTree->setHeaderHidden(true);
    m_categoryTree->setIndentation(14);
    m_categoryTree->setMinimumWidth(180);

    QTreeWidgetItem *mediaSection = makeSectionHeader(m_categoryTree, tr("MEDIA"));
    makeCategory(mediaSection, tr("Videos"), IconFactory::treeVideo());
    QTreeWidgetItem *imagesItem = makeCategory(mediaSection, tr("Images"), IconFactory::treeImage());
    makeCategory(mediaSection, tr("Feeds"), IconFactory::treeFeed());
    makeCategory(mediaSection, tr("DVD"), IconFactory::treeDvd());
    makeCategory(mediaSection, tr("Audio"), IconFactory::treeAudio());

    QTreeWidgetItem *collectionsSection = makeSectionHeader(m_categoryTree, tr("COLLECTIONS"));
    auto *noCollectionsYet = new QTreeWidgetItem(collectionsSection, {tr("No collections yet")});
    noCollectionsYet->setFlags(noCollectionsYet->flags() & ~Qt::ItemIsSelectable & ~Qt::ItemIsEnabled);
    QFont italic = noCollectionsYet->font(0);
    italic.setItalic(true);
    noCollectionsYet->setFont(0, italic);

    m_categoryTree->expandItem(mediaSection);
    m_categoryTree->collapseItem(collectionsSection);
    connect(m_categoryTree, &QTreeWidget::itemClicked, this, &MediaLibraryPanel::onCategorySelected);

    // Import control -- deliberately the same "+" button regardless of
    // which category is selected; only its tooltip/filter changes.
    m_importButton = new QToolButton(mediaTab);
    m_importButton->setText(QStringLiteral("+"));
    m_importButton->setToolTip(tr("Import files into this category"));
    connect(m_importButton, &QToolButton::clicked, this, &MediaLibraryPanel::onImportClicked);

    auto *treeFooterLayout = new QHBoxLayout();
    treeFooterLayout->setContentsMargins(2, 2, 2, 2);
    treeFooterLayout->addWidget(m_importButton);
    treeFooterLayout->addStretch(1);

    auto *treePanel = new QWidget(mediaTab);
    auto *treePanelLayout = new QVBoxLayout(treePanel);
    treePanelLayout->setContentsMargins(0, 0, 0, 0);
    treePanelLayout->setSpacing(0);
    treePanelLayout->addWidget(m_categoryTree, 1);
    treePanelLayout->addLayout(treeFooterLayout);

    // Grid panel (with the Title / File Name header row from the reference)
    auto *gridHeaderLayout = new QHBoxLayout();
    auto *titleHeader = new QLabel(tr("Title"), mediaTab);
    auto *fileNameHeader = new QLabel(tr("File Name"), mediaTab);
    fileNameHeader->setStyleSheet("color: #888;");
    titleHeader->setStyleSheet("color: #888;");
    gridHeaderLayout->addWidget(titleHeader);
    gridHeaderLayout->addStretch(1);
    gridHeaderLayout->addWidget(fileNameHeader);

    m_mediaGrid = new QListWidget(mediaTab);
    m_mediaGrid->setViewMode(QListView::IconMode);
    m_mediaGrid->setIconSize(QSize(kGridIconSize, kGridIconSize));
    m_mediaGrid->setGridSize(QSize(kGridIconSize + 24, kGridIconSize + 34));
    m_mediaGrid->setResizeMode(QListView::Adjust);
    m_mediaGrid->setMovement(QListView::Static);
    m_mediaGrid->setSpacing(6);
    m_mediaGrid->setWordWrap(true);
    m_mediaGrid->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_mediaGrid, &QListWidget::currentItemChanged, this, &MediaLibraryPanel::onGridSelectionChanged);
    connect(m_mediaGrid, &QListWidget::itemActivated, this, &MediaLibraryPanel::onGridItemActivated);
    connect(m_mediaGrid, &QListWidget::itemDoubleClicked, this, &MediaLibraryPanel::onGridItemActivated);
    connect(m_mediaGrid, &QListWidget::customContextMenuRequested, this, &MediaLibraryPanel::onGridContextMenu);

    auto *gridPanel = new QWidget(mediaTab);
    auto *gridPanelLayout = new QVBoxLayout(gridPanel);
    gridPanelLayout->setContentsMargins(0, 0, 0, 0);
    gridPanelLayout->addLayout(gridHeaderLayout);
    gridPanelLayout->addWidget(m_mediaGrid, 1);

    // Preview panel
    m_previewImage = new QLabel(mediaTab);
    m_previewImage->setMinimumSize(kPreviewSize, kPreviewSize * 9 / 16);
    m_previewImage->setAlignment(Qt::AlignCenter);
    m_previewImage->setStyleSheet("background-color: #0f0f11; border: 1px solid #333;");
    m_previewImage->setScaledContents(false);
    m_previewCaption = new QLabel(mediaTab);
    m_previewCaption->setAlignment(Qt::AlignCenter);
    m_previewCaption->setStyleSheet("color: #cfcfcf; padding-top: 4px;");

    auto *previewPanel = new QWidget(mediaTab);
    auto *previewPanelLayout = new QVBoxLayout(previewPanel);
    previewPanelLayout->setContentsMargins(0, 0, 0, 0);
    previewPanelLayout->addWidget(m_previewImage);
    previewPanelLayout->addWidget(m_previewCaption);
    previewPanelLayout->addStretch(1);

    // All three columns sit in a splitter so the user can resize them
    // however they like -- same convention as the rest of the app.
    auto *mediaSplitter = new QSplitter(mediaTab);
    mediaSplitter->addWidget(treePanel);
    mediaSplitter->addWidget(gridPanel);
    mediaSplitter->addWidget(previewPanel);
    mediaSplitter->setStretchFactor(0, 1);
    mediaSplitter->setStretchFactor(1, 3);
    mediaSplitter->setStretchFactor(2, 2);

    m_itemCountLabel = new QLabel(this);
    m_itemCountLabel->setStyleSheet("color: #888; padding: 2px 6px;");
    m_itemCountLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    auto *mediaTabLayout = new QVBoxLayout(mediaTab);
    mediaTabLayout->addWidget(m_searchBox);
    mediaTabLayout->addWidget(mediaSplitter, 1);

    m_contentTabs->addTab(mediaTab, tr("Media"));
    m_contentTabs->addTab(new QWidget(this), tr("Presentations"));
    m_contentTabs->addTab(new QWidget(this), tr("Themes"));

    const int mediaTabIndex = m_contentTabs->indexOf(mediaTab);
    m_contentTabs->setTabEnabled(0, false); // Songs
    m_contentTabs->setTabEnabled(mediaTabIndex + 1, false); // Presentations
    m_contentTabs->setTabEnabled(mediaTabIndex + 2, false); // Themes
    Q_UNUSED(scripturesTabIndex); // enabled by default; kept for clarity at the call site above
    m_contentTabs->setCurrentIndex(mediaTabIndex);

    for (int i = 0; i < m_contentTabs->count(); ++i) {
        if (!m_contentTabs->isTabEnabled(i))
            m_contentTabs->setTabToolTip(i, tr("Coming in a future update"));
    }

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(2);
    rootLayout->addWidget(m_contentTabs, 1);
    rootLayout->addWidget(m_itemCountLabel);

    m_categoryTree->setCurrentItem(imagesItem);
    m_currentCategory = tr("Images");
    rebuildGridForCategory(m_currentCategory);
}

void MediaLibraryPanel::populateSampleMedia()
{
    // Placeholder sample data -- original flat-color swatches (generated
    // in code, not real photography) standing in until the user imports
    // their own files via the "+" button.
    m_sampleData[tr("Images")] = {
        {tr("Beach Sunset"), QColor("#e08a4f"), QString()},
        {tr("Blue Paint"), QColor("#1b2a6b"), QString()},
        {tr("Cross Sunset"), QColor("#8e3b46"), QString()},
        {tr("Fall Aspen"), QColor("#cfd66b"), QString()},
        {tr("Highway"), QColor("#5b7a9d"), QString()},
        {tr("Leaves"), QColor("#c98a3a"), QString()},
        {tr("Mountain Lake"), QColor("#3e6e6e"), QString()},
        {tr("Sun and Clouds"), QColor("#bcd7e6"), QString()},
        {tr("Tree"), QColor("#3f6b3f"), QString()},
        {tr("Yellow Sky"), QColor("#d8c93a"), QString()},
    };
    m_sampleData[tr("Videos")] = {};
    m_sampleData[tr("Feeds")] = {};
    m_sampleData[tr("DVD")] = {};
    m_sampleData[tr("Audio")] = {};
}

QIcon MediaLibraryPanel::iconForEntry(const MediaEntry &entry) const
{
    if (!entry.imagePath.isEmpty())
        return QIcon(makeImageThumbnail(entry.imagePath, kGridIconSize, entry.focus));
    return QIcon(makeSwatch(entry.color, kGridIconSize));
}

void MediaLibraryPanel::onCategorySelected(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    if (!item || !item->parent())
        return; // ignore clicks on non-selectable section headers

    m_currentCategory = item->text(0);
    rebuildGridForCategory(m_currentCategory);
}

void MediaLibraryPanel::rebuildGridForCategory(const QString &category)
{
    m_mediaGrid->clear();

    const auto items = m_sampleData.value(category);
    for (int i = 0; i < items.size(); ++i) {
        const MediaEntry &entry = items.at(i);
        auto *listItem = new QListWidgetItem(iconForEntry(entry), entry.label);
        listItem->setData(Qt::UserRole, entry.color);
        listItem->setData(Qt::UserRole + 1, entry.imagePath);
        listItem->setData(Qt::UserRole + 2, entry.focus);
        // Index into m_sampleData[category] this item corresponds to --
        // lets the context menu write an edited focus point straight
        // back to the source-of-truth entry rather than needing to
        // search for it by label (which need not be unique).
        listItem->setData(Qt::UserRole + 3, i);
        listItem->setTextAlignment(Qt::AlignHCenter);
        m_mediaGrid->addItem(listItem);
    }

    if (items.isEmpty()) {
        m_itemCountLabel->setText(tr("No items yet"));
        m_previewImage->setPixmap(QPixmap());
        m_previewCaption->clear();
    } else {
        m_itemCountLabel->setText(items.size() == 1 ? tr("1 item") : tr("%1 items").arg(items.size()));
        m_mediaGrid->setCurrentRow(0);
    }
}

void MediaLibraryPanel::onGridSelectionChanged()
{
    QListWidgetItem *item = m_mediaGrid->currentItem();
    if (!item) {
        m_previewImage->setPixmap(QPixmap());
        m_previewCaption->clear();
        return;
    }

    const QString imagePath = item->data(Qt::UserRole + 1).toString();
    if (!imagePath.isEmpty()) {
        const QPixmap source(imagePath);
        m_previewImage->setPixmap(source.scaled(m_previewImage->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        const QColor color = item->data(Qt::UserRole).value<QColor>();
        m_previewImage->setPixmap(makeSwatch(color, kPreviewSize));
    }
    m_previewCaption->setText(item->text());
}

void MediaLibraryPanel::onGridItemActivated(QListWidgetItem *item)
{
    if (!item)
        return;
    const QColor color = item->data(Qt::UserRole).value<QColor>();
    const QString imagePath = item->data(Qt::UserRole + 1).toString();
    const QPointF focus = item->data(Qt::UserRole + 2).toPointF();
    emit mediaActivated(item->text(), color, imagePath, focus);
}

void MediaLibraryPanel::onGridContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = m_mediaGrid->itemAt(pos);
    if (!item)
        return;

    const QString label = item->text();
    const QColor color = item->data(Qt::UserRole).value<QColor>();
    const QString imagePath = item->data(Qt::UserRole + 1).toString();
    const QPointF focus = item->data(Qt::UserRole + 2).toPointF();
    const int entryIndex = item->data(Qt::UserRole + 3).toInt();

    QMenu menu(this);
    QAction *goLiveAction = menu.addAction(tr("Go Live"));
    QAction *sendToPhoneAction = menu.addAction(tr("Send to Phone Only"));
    menu.addSeparator();
    QAction *editFramingAction = menu.addAction(tr("Edit Framing..."));
    // Framing only means something for a real photo -- the color
    // swatches have nothing to crop.
    editFramingAction->setEnabled(!imagePath.isEmpty());

    QAction *chosen = menu.exec(m_mediaGrid->viewport()->mapToGlobal(pos));
    if (chosen == goLiveAction) {
        emit mediaActivated(label, color, imagePath, focus);
    } else if (chosen == sendToPhoneAction) {
        emit mediaSentToPhone(label, color, imagePath, focus);
    } else if (chosen == editFramingAction) {
        ImageFramingDialog dialog(imagePath, focus, this);
        if (dialog.exec() != QDialog::Accepted)
            return;

        const QPointF newFocus = dialog.focus();
        auto &entries = m_sampleData[m_currentCategory];
        if (entryIndex >= 0 && entryIndex < entries.size()) {
            entries[entryIndex].focus = newFocus;
            item->setData(Qt::UserRole + 2, newFocus);
            item->setIcon(iconForEntry(entries.at(entryIndex)));
            if (item == m_mediaGrid->currentItem())
                onGridSelectionChanged(); // refresh the larger preview too
        }
    }
}

void MediaLibraryPanel::onImportClicked()
{
    const QString filter = fileFilterForCategory(m_currentCategory);
    const QStringList paths = QFileDialog::getOpenFileNames(
        this, tr("Import into %1").arg(m_currentCategory), QString(), filter);
    if (paths.isEmpty())
        return;

    auto &entries = m_sampleData[m_currentCategory];
    const bool isImageCategory = (m_currentCategory == tr("Images"));

    for (const QString &path : paths) {
        const QString label = QFileInfo(path).completeBaseName();
        if (isImageCategory) {
            entries.append({label, QColor(), path});
        } else {
            // No thumbnailing/playback for these types yet -- store a
            // stable placeholder color so the entry is at least visually
            // distinct and re-selectable; the file path is still kept.
            entries.append({label, colorFromName(label), path});
        }
    }

    rebuildGridForCategory(m_currentCategory);
    m_mediaGrid->setCurrentRow(m_mediaGrid->count() - 1);
}