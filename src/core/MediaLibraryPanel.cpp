#include "MediaLibraryPanel.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPainter>
#include <QSplitter>
#include <QTabWidget>
#include <QToolButton>
#include <QTreeWidget>

#include "IconFactory.h"

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

// A category header row ("MEDIA", "ONLINE", "COLLECTIONS") -- visible for
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
}

MediaLibraryPanel::MediaLibraryPanel(QWidget *parent) : QWidget(parent)
{
    populateSampleMedia();

    // ---------- Content-type tab row ----------
    m_contentTabs = new QTabWidget(this);

    m_contentTabs->addTab(new QWidget(this), tr("Songs"));
    m_contentTabs->addTab(new QWidget(this), tr("Scriptures"));

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

    QTreeWidgetItem *onlineSection = makeSectionHeader(m_categoryTree, tr("ONLINE"));
    makeCategory(onlineSection, tr("Premium Media"), IconFactory::treePremium());

    QTreeWidgetItem *collectionsSection = makeSectionHeader(m_categoryTree, tr("COLLECTIONS"));
    auto *noCollectionsYet = new QTreeWidgetItem(collectionsSection, {tr("No collections yet")});
    noCollectionsYet->setFlags(noCollectionsYet->flags() & ~Qt::ItemIsSelectable & ~Qt::ItemIsEnabled);
    QFont italic = noCollectionsYet->font(0);
    italic.setItalic(true);
    noCollectionsYet->setFont(0, italic);

    m_categoryTree->expandItem(mediaSection);
    m_categoryTree->expandItem(onlineSection);
    m_categoryTree->collapseItem(collectionsSection);
    connect(m_categoryTree, &QTreeWidget::itemClicked, this, &MediaLibraryPanel::onCategorySelected);

    auto *treePanel = new QWidget(mediaTab);
    auto *treePanelLayout = new QVBoxLayout(treePanel);
    treePanelLayout->setContentsMargins(0, 0, 0, 0);
    treePanelLayout->addWidget(m_categoryTree);

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
    connect(m_mediaGrid, &QListWidget::currentItemChanged, this, &MediaLibraryPanel::onGridSelectionChanged);
    connect(m_mediaGrid, &QListWidget::itemActivated, this, &MediaLibraryPanel::onGridItemActivated);
    connect(m_mediaGrid, &QListWidget::itemDoubleClicked, this, &MediaLibraryPanel::onGridItemActivated);

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
    m_contentTabs->setTabEnabled(1, false); // Scriptures
    m_contentTabs->setTabEnabled(mediaTabIndex + 1, false); // Presentations
    m_contentTabs->setTabEnabled(mediaTabIndex + 2, false); // Themes
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
    rebuildGridForCategory(tr("Images"));
}

void MediaLibraryPanel::populateSampleMedia()
{
    // Placeholder sample data -- original flat-color swatches (generated
    // in code, not real photography) standing in for a real media
    // library until file import is built (build-plan step 6).
    m_sampleData[tr("Images")] = {
        {tr("Beach Sunset"), QColor("#e08a4f")},
        {tr("Blue Paint"), QColor("#1b2a6b")},
        {tr("Cross Sunset"), QColor("#8e3b46")},
        {tr("Fall Aspen"), QColor("#cfd66b")},
        {tr("Highway"), QColor("#5b7a9d")},
        {tr("Leaves"), QColor("#c98a3a")},
        {tr("Mountain Lake"), QColor("#3e6e6e")},
        {tr("Sun and Clouds"), QColor("#bcd7e6")},
        {tr("Tree"), QColor("#3f6b3f")},
        {tr("Yellow Sky"), QColor("#d8c93a")},
    };
    m_sampleData[tr("Videos")] = {};
    m_sampleData[tr("Feeds")] = {};
    m_sampleData[tr("DVD")] = {};
    m_sampleData[tr("Audio")] = {};
    m_sampleData[tr("Premium Media")] = {};
}

void MediaLibraryPanel::onCategorySelected(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    if (!item || !item->parent())
        return; // ignore clicks on non-selectable section headers

    rebuildGridForCategory(item->text(0));
}

void MediaLibraryPanel::rebuildGridForCategory(const QString &category)
{
    m_mediaGrid->clear();

    const auto items = m_sampleData.value(category);
    for (const auto &entry : items) {
        auto *listItem = new QListWidgetItem(QIcon(makeSwatch(entry.second, kGridIconSize)), entry.first);
        listItem->setData(Qt::UserRole, entry.second);
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

    const QColor color = item->data(Qt::UserRole).value<QColor>();
    m_previewImage->setPixmap(makeSwatch(color, kPreviewSize));
    m_previewCaption->setText(item->text());
}

void MediaLibraryPanel::onGridItemActivated(QListWidgetItem *item)
{
    if (!item)
        return;
    const QColor color = item->data(Qt::UserRole).value<QColor>();
    emit mediaActivated(item->text(), color);
}