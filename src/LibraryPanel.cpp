#include "LibraryPanel.h"
#include "Theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTabWidget>
#include <QTreeWidget>
#include <QListWidget>
#include <QLabel>
#include <QPixmap>
#include <QPainter>

namespace {

// Generates a simple placeholder thumbnail (flat-colored swatch) since no
// real media assets are wired up yet - this is UI scaffolding only.
QPixmap placeholderThumb(const QColor& color, const QSize& size = QSize(110, 74))
{
    QPixmap pix(size);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    QLinearGradient grad(0, 0, 0, size.height());
    grad.setColorAt(0, color.lighter(120));
    grad.setColorAt(1, color.darker(140));
    p.setPen(Qt::NoPen);
    p.setBrush(grad);
    p.drawRoundedRect(pix.rect().adjusted(1, 1, -1, -1), 6, 6);
    return pix;
}

} // namespace

LibraryPanel::LibraryPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    m_tabs = new QTabWidget(this);
    m_tabs->addTab(new QWidget, "Songs");
    m_tabs->addTab(new QWidget, "Scriptures");
    m_tabs->addTab(buildMediaTab(), "Media");
    m_tabs->addTab(new QWidget, "Presentations");
    m_tabs->addTab(new QWidget, "Themes");
    m_tabs->setCurrentIndex(2); // "Media" active, matching the reference layout

    outer->addWidget(m_tabs, 1);
}

QWidget* LibraryPanel::buildMediaTab()
{
    auto* mediaTab = new QWidget;
    auto* layout = new QHBoxLayout(mediaTab);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* splitter = new QSplitter(Qt::Horizontal, mediaTab);
    splitter->setChildrenCollapsible(false);

    // Folder navigation tree.
    m_folderTree = new QTreeWidget;
    m_folderTree->setHeaderHidden(true);
    m_folderTree->setMinimumWidth(150);
    populateFolderTree();

    // Thumbnail grid.
    m_thumbGrid = new QListWidget;
    m_thumbGrid->setViewMode(QListView::IconMode);
    m_thumbGrid->setIconSize(QSize(110, 74));
    m_thumbGrid->setResizeMode(QListView::Adjust);
    m_thumbGrid->setMovement(QListView::Static);
    m_thumbGrid->setSpacing(12);
    m_thumbGrid->setUniformItemSizes(true);
    populateThumbnails();

    splitter->addWidget(m_folderTree);
    splitter->addWidget(m_thumbGrid);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({160, 480});

    layout->addWidget(splitter);
    return mediaTab;
}

void LibraryPanel::populateFolderTree()
{
    const QStringList folders = {
        "Videos", "Images", "Feeds", "DVD", "Time", "Audio", "Collections"
    };
    for (const auto& f : folders) {
        auto* item = new QTreeWidgetItem(m_folderTree, {f});
        if (f == "Images") {
            item->setForeground(0, QColor(Theme::kAccentBlue));
            m_folderTree->setCurrentItem(item);
        }
    }
}

void LibraryPanel::populateThumbnails()
{
    struct Entry { const char* name; QColor color; };
    const Entry entries[] = {
        {"Beach Sunset",   QColor(210, 90, 40)},
        {"Blue Paint",     QColor(40, 90, 190)},
        {"Cross Sunset",   QColor(150, 40, 30)},
        {"Tree",           QColor(70, 130, 60)},
        {"Yellow Sky",     QColor(200, 150, 40)},
        {"Sun and Clouds", QColor(150, 170, 190)},
    };
    for (const auto& e : entries) {
        auto* item = new QListWidgetItem(QIcon(placeholderThumb(e.color)), e.name);
        item->setTextAlignment(Qt::AlignHCenter);
        m_thumbGrid->addItem(item);
    }
    if (m_thumbGrid->count() > 0)
        m_thumbGrid->setCurrentRow(0);
}
