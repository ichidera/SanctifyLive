#include "ThemePanel.h"

#include <QColorDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QSplitter>
#include <QToolButton>
#include <QVBoxLayout>

#include "../common/IconFactory.h"
#include "../output/LiveAppearancePreview.h"
#include "../schedule/Slide.h"

namespace {
constexpr int kGridIconSize = 64;

// Same rounded-swatch look as MediaLibraryPanel's built-in placeholder
// entries -- duplicated rather than shared, matching how this codebase
// already keeps small per-panel visual helpers local to their .cpp
// (compare MediaLibraryPanel.cpp's and ScripturePanel.cpp's own separate
// makeSectionHeader()).
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
}

ThemePanel::ThemePanel(QWidget *parent) : QWidget(parent)
{
    populateBuiltInThemes();

    m_grid = new QListWidget(this);
    m_grid->setViewMode(QListView::IconMode);
    m_grid->setIconSize(QSize(kGridIconSize, kGridIconSize));
    m_grid->setGridSize(QSize(kGridIconSize + 24, kGridIconSize + 34));
    m_grid->setResizeMode(QListView::Adjust);
    m_grid->setMovement(QListView::Static);
    m_grid->setSpacing(6);
    m_grid->setWordWrap(true);
    connect(m_grid, &QListWidget::currentItemChanged, this, &ThemePanel::onSelectionChanged);
    connect(m_grid, &QListWidget::itemActivated, this, &ThemePanel::onItemActivated);
    connect(m_grid, &QListWidget::itemDoubleClicked, this, &ThemePanel::onItemActivated);

    m_addButton = new QToolButton(this);
    m_addButton->setText(QStringLiteral("+"));
    m_addButton->setToolTip(tr("Add a solid-color or image theme"));
    connect(m_addButton, &QToolButton::clicked, this, &ThemePanel::onAddThemeClicked);

    m_itemCountLabel = new QLabel(this);
    m_itemCountLabel->setStyleSheet("color: #888; padding: 2px 6px;");

    auto *gridFooter = new QHBoxLayout();
    gridFooter->addWidget(m_addButton);
    gridFooter->addStretch(1);
    gridFooter->addWidget(m_itemCountLabel);

    auto *gridPanel = new QWidget(this);
    auto *gridPanelLayout = new QVBoxLayout(gridPanel);
    gridPanelLayout->setContentsMargins(0, 0, 0, 0);
    gridPanelLayout->addWidget(m_grid, 1);
    gridPanelLayout->addLayout(gridFooter);

    m_preview = new LiveAppearancePreview(this);

    auto *splitter = new QSplitter(this);
    splitter->addWidget(gridPanel);
    splitter->addWidget(m_preview);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(4, 4, 4, 4);
    rootLayout->addWidget(splitter, 1);

    rebuildGrid();
    if (m_grid->count() > 0)
        m_grid->setCurrentRow(0);
}

void ThemePanel::populateBuiltInThemes()
{
    // Flat-color placeholders, same idea as MediaLibraryPanel's built-in
    // sample media -- enough to prove the layout and preview before an
    // operator adds their own via "+".
    m_themes = {
        {tr("Midnight Blue"), QColor("#0b1b3a"), QString()},
        {tr("Warm Charcoal"), QColor("#2a2622"), QString()},
        {tr("Deep Maroon"), QColor("#3a0f14"), QString()},
        {tr("Forest"), QColor("#123321"), QString()},
        {tr("Slate"), QColor("#26333d"), QString()},
        {tr("Plain Black"), QColor("#000000"), QString()},
    };
}

QIcon ThemePanel::iconForEntry(const ThemeEntry &entry) const
{
    if (!entry.imagePath.isEmpty())
        return QIcon(entry.imagePath);
    return QIcon(makeSwatch(entry.color, kGridIconSize));
}

void ThemePanel::rebuildGrid()
{
    m_grid->clear();
    for (int i = 0; i < m_themes.size(); ++i) {
        const ThemeEntry &entry = m_themes.at(i);
        auto *item = new QListWidgetItem(iconForEntry(entry), entry.name);
        item->setData(Qt::UserRole, i);
        item->setTextAlignment(Qt::AlignHCenter);
        m_grid->addItem(item);
    }
    m_itemCountLabel->setText(m_themes.size() == 1 ? tr("1 theme") : tr("%1 themes").arg(m_themes.size()));
}

void ThemePanel::setOutputProfile(const OutputProfile &profile)
{
    m_preview->setProfile(profile);
}

void ThemePanel::onSelectionChanged()
{
    QListWidgetItem *item = m_grid->currentItem();
    if (!item) {
        m_preview->clearSlide();
        return;
    }
    const int index = item->data(Qt::UserRole).toInt();
    if (index < 0 || index >= m_themes.size()) {
        m_preview->clearSlide();
        return;
    }
    const ThemeEntry &entry = m_themes.at(index);
    m_preview->setSlide(Slide::fromMediaEntry(entry.name, entry.color, entry.imagePath, entry.focus));
}

void ThemePanel::onItemActivated(QListWidgetItem *item)
{
    if (!item)
        return;
    const int index = item->data(Qt::UserRole).toInt();
    if (index < 0 || index >= m_themes.size())
        return;
    const ThemeEntry &entry = m_themes.at(index);
    emit themeActivated(entry.name, entry.color, entry.imagePath, entry.focus);
}

void ThemePanel::onAddThemeClicked()
{
    QMenu menu(this);
    QAction *colorAction = menu.addAction(tr("Solid Color Theme..."));
    QAction *imageAction = menu.addAction(tr("Image Theme..."));
    QAction *chosen = menu.exec(m_addButton->mapToGlobal(QPoint(0, m_addButton->height())));
    if (!chosen)
        return;

    if (chosen == colorAction) {
        const QColor color = QColorDialog::getColor(Qt::black, this, tr("Choose Theme Color"));
        if (!color.isValid())
            return;
        bool ok = false;
        const QString name = QInputDialog::getText(this, tr("Name This Theme"), tr("Theme name:"),
                                                     QLineEdit::Normal, tr("New Theme"), &ok);
        if (!ok || name.trimmed().isEmpty())
            return;
        m_themes.append({name.trimmed(), color, QString()});
    } else if (chosen == imageAction) {
        const QString path = QFileDialog::getOpenFileName(
            this, tr("Choose Theme Background Image"), QString(),
            tr("Images (*.png *.jpg *.jpeg *.bmp *.gif);;All Files (*)"));
        if (path.isEmpty())
            return;
        m_themes.append({QFileInfo(path).completeBaseName(), QColor(), path});
    }

    rebuildGrid();
    m_grid->setCurrentRow(m_grid->count() - 1);
}
