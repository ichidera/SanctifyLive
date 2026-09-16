#pragma once

#include <QWidget>

class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QTableView;
class QLabel;
class QModelIndex;
class ScriptureTableModel;

// ScripturePanel is the SCRIPTURES tab (one of the Content Tabs -- see
// OperatorWindow's panel glossary): a translation checklist on the
// left, a searchable Reference/Scripture table on the right, and a
// bottom bar with a live reference count -- matching the reference
// layout's Songs/Scriptures/Media/Presentations/Themes strip.
//
// Only KJV is real. It ships bundled with the app (see
// core/scripture/ScriptureLibrary) rather than needing any lookup or
// import step. HCSB and RVA are shown in the translation checklist --
// same as the target design -- but greyed out and unselectable: there's
// no Store yet to actually deliver them (see README roadmap), so
// showing them as if they worked would be a lie the same way a fake
// "video preview" thumbnail would be in the Media tab. "More
// Available..." is a real, clickable link (not a disabled stub) since
// clicking it is honest about what it does today -- it tells the
// operator where more translations will come from once the Store
// exists, rather than pretending to open one now.
//
// Like MediaLibraryPanel, this panel does NOT render its own preview --
// the "how would this actually look" rendering lives one level up, in
// OperatorWindow's Item Preview panel (next to History). This panel's
// only job is to tell OperatorWindow *what* to preview, via
// previewRequested(), and what to add to Schedule, via
// scriptureActivated().
//
// The verse table is backed by ScriptureTableModel (a QAbstractTableModel
// over ScriptureLibrary::verses(), defined in this .cpp) rather than a
// QTableWidget populated with 31,000+ QTableWidgetItems -- with a
// translation's full text loaded, a real model/view split is what keeps
// scrolling and filtering responsive.
class ScripturePanel : public QWidget
{
    Q_OBJECT

public:
    explicit ScripturePanel(QWidget *parent = nullptr);

signals:
    // Emitted when the operator double-clicks (or presses Enter/Return
    // on) a verse row. `reference` is shown to the user (e.g. as the
    // new slide's label); `text` is the verse body that gets projected.
    void scriptureActivated(const QString &reference, const QString &text);

    // Emitted on a single click/press of a verse row, before any
    // double-click has a chance to register -- mirrors
    // MediaLibraryPanel::previewRequested(). OperatorWindow uses this to
    // update its Item Preview panel without anything being added to the
    // Schedule.
    void previewRequested(const QString &reference, const QString &text);

private:
    QWidget *buildSearchBar();
    QWidget *buildTranslationsColumn();
    QWidget *buildBottomBar();

    void onSearchTextChanged(const QString &text);
    void onTranslationItemChanged(QListWidgetItem *item);
    void onMoreAvailableClicked();
    void onRowClicked(const QModelIndex &index);
    void onRowActivated(const QModelIndex &index);
    void refreshReferenceCount();

    QLineEdit *m_searchEdit = nullptr;
    QListWidget *m_translationsList = nullptr;
    QListWidgetItem *m_kjvItem = nullptr;
    QTableView *m_table = nullptr;
    ScriptureTableModel *m_model = nullptr;
    QLabel *m_referenceCountLabel = nullptr;
};
