#ifndef SANCTIFYLIVE_CORE_THEME_THEMEPANEL_H_
#define SANCTIFYLIVE_CORE_THEME_THEMEPANEL_H_

#include <QColor>
#include <QIcon>
#include <QPointF>
#include <QVector>
#include <QWidget>

#include "../settings/OutputProfile.h"

class QListWidget;
class QListWidgetItem;
class QToolButton;
class QLabel;
class LiveAppearancePreview;

// ThemePanel: the "Themes" content-type tab of the bottom resource
// library. A theme here is deliberately minimal for this first
// milestone (same spirit as Slide itself, see src/core/schedule/Slide.h)
// -- a background (solid color or image) plus the sample text used to
// preview it -- rather than the fuller "font/size/position/shadow
// per-theme" model this will eventually grow into.
//
// Selecting a theme drives the shared LiveAppearancePreview exactly the
// way Media and Scriptures do, so an operator can see precisely how a
// theme's background will crop/fit the actual configured output before
// using it. "Go Live"/double-click sends it live as a full-screen
// background, the same as a Media entry -- ThemePanel reuses
// Slide::fromMediaEntry (see src/core/schedule/Slide.h) and emits the
// same argument shape as MediaLibraryPanel::mediaActivated so
// OperatorWindow can wire both into the same slot.
class ThemePanel : public QWidget
{
    Q_OBJECT

public:
    explicit ThemePanel(QWidget *parent = nullptr);

signals:
    // Same payload shape as MediaLibraryPanel::mediaActivated -- see that
    // signal's doc comment for what each argument means.
    void themeActivated(const QString &label, const QColor &background, const QString &imagePath,
                         const QPointF &focus);

public slots:
    void setOutputProfile(const OutputProfile &profile);

private slots:
    void onSelectionChanged();
    void onItemActivated(QListWidgetItem *item);
    void onAddThemeClicked();

private:
    struct ThemeEntry
    {
        QString name;
        QColor color;
        QString imagePath; // empty for solid-color themes
        QPointF focus = QPointF(0.5, 0.5);
    };

    void populateBuiltInThemes();
    void rebuildGrid();
    QIcon iconForEntry(const ThemeEntry &entry) const;

    QListWidget *m_grid;
    LiveAppearancePreview *m_preview;
    QToolButton *m_addButton;
    QLabel *m_itemCountLabel;

    QVector<ThemeEntry> m_themes;
};

#endif // SANCTIFYLIVE_CORE_THEME_THEMEPANEL_H_
