#pragma once

#include <QVector>
#include <QWidget>

#include "core/scripture/ScriptureReference.h"

class QHBoxLayout;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QTableView;
class QToolButton;
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
// SEARCH has two distinct modes (a real implementation of the two
// search styles most Bible-presentation tools offer -- EasyWorship
// calls them "keyword search" and "Scripture reference"), switched via
// the Words/Reference toggle at the top of the search bar:
//
//   Words mode filters the verse table down to every verse whose text
//   contains all the typed words (case-insensitive, AND'd together) --
//   a plain "search the Bible's contents" a la a search engine.
//
//   Reference mode is a progressively-constrained navigator, not a
//   filter: it walks Book -> Chapter -> Verse -> optional end-Verse in
//   that order, because that's the only order the grammar makes sense
//   in (a verse number means nothing until a chapter is known, a
//   chapter means nothing until a book is known). While the book is
//   still ambiguous ("J"), matching book names appear as suggestion
//   chips below the search box; once resolved, a hint line reports the
//   valid range for whatever comes next ("John -- enter a chapter
//   (1-21)") and rejects anything outside it ("John 999", "John 3:99")
//   with a plain-language reason instead of silently failing. The
//   underlying grammar and suggestion logic live in
//   core/scripture/ScriptureReference, kept separate from this widget
//   so they're unit-testable without a QApplication. Reference mode
//   never filters the table -- it selects/scrolls to the resolved
//   verse(s) in the full list, matching how the target design keeps
//   "31,102 references" visible in the corner while "Genesis 1:1" sits
//   in the search box.
//
// SELECTION: the verse table allows a contiguous multi-row selection
// (shift-click, shift-arrow, or click-drag), not just one row at a
// time. A single selected verse previews/adds as itself; multiple
// selected verses are compressed into ONE slide, combining their text
// and collapsing the reference into a range (e.g. "Esther 8:9-10")
// rather than sending each verse to Schedule separately -- see
// ScriptureTableModel::combinedReferenceAndText() in the .cpp for the
// exact grouping rule non-contiguous verses fall back to (this
// matters once Words-mode search results, which aren't necessarily
// Bible-consecutive, are multi-selected).
//
// Like MediaLibraryPanel, this panel does NOT render its own preview --
// the "how would this actually look" rendering lives one level up, in
// OperatorWindow's Item Preview panel (next to History). This panel's
// only job is to tell OperatorWindow *what* to preview, via
// previewRequested(), and what to add to Schedule, via
// scriptureActivated(). A single click (or arrow-key move, or
// shift-extending a range) fires previewRequested(); a double-click,
// Enter/Return in the table, or Enter/Return in the search box fires
// scriptureActivated() for whatever's currently selected.
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
    // Emitted when the operator commits the current selection to the
    // live Schedule (double-click/Enter on the table, or Enter in the
    // search box with a resolved reference). `reference` is shown to
    // the user (e.g. as the new slide's label) -- a range like "Esther
    // 8:9-10" when multiple verses were selected; `text` is the
    // (possibly multi-verse) body that gets projected.
    void scriptureActivated(const QString &reference, const QString &text);

    // Emitted whenever the current selection changes (click, arrow
    // keys, a resolved Reference-mode search, etc.) -- mirrors
    // MediaLibraryPanel::previewRequested(). OperatorWindow uses this to
    // update its Item Preview panel without anything being added to the
    // Schedule.
    void previewRequested(const QString &reference, const QString &text);

private:
    enum class SearchMode
    {
        Words,
        Reference,
    };

    QWidget *buildSearchBar();
    QWidget *buildReferenceHintArea();
    QWidget *buildTranslationsColumn();
    QWidget *buildBottomBar();

    void setMode(SearchMode mode);
    void onSearchTextChanged(const QString &text);
    void updateWordsMode(const QString &text);
    void updateReferenceMode(const QString &text);
    void updateHintLabel(const ScriptureReference::Parsed &parsed);
    void rebuildSuggestionChips(const QVector<ScriptureReference::BookSuggestion> &suggestions);
    void applySelection(int firstSourceRow, int lastSourceRow);
    void onResetClicked();
    void onTranslationItemChanged(QListWidgetItem *item);
    void onMoreAvailableClicked();
    void emitForSelection(bool activate);
    QVector<int> selectedSourceRowsSorted() const;
    void refreshReferenceCount();

    SearchMode m_mode = SearchMode::Reference;

    QToolButton *m_wordsModeButton = nullptr;
    QToolButton *m_referenceModeButton = nullptr;
    QLineEdit *m_searchEdit = nullptr;
    QToolButton *m_resetButton = nullptr;

    QLabel *m_hintLabel = nullptr;
    QWidget *m_suggestionsRow = nullptr;
    QHBoxLayout *m_suggestionsLayout = nullptr;

    QListWidget *m_translationsList = nullptr;
    QListWidgetItem *m_kjvItem = nullptr;
    QPushButton *m_moreAvailableButton = nullptr;

    QTableView *m_table = nullptr;
    ScriptureTableModel *m_model = nullptr;
    QLabel *m_referenceCountLabel = nullptr;
};
