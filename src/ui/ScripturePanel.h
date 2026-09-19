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
// OperatorWindow's panel glossary): a translation list on the left, a
// searchable Reference/Scripture table on the right, and a bottom bar
// with a live reference count -- matching the reference layout's
// Songs/Scriptures/Media/Presentations/Themes strip.
//
// TRANSLATIONS: four real, fully bundled translations -- KJV, ASV,
// NHEB, and RVA (Spanish) -- all public domain (see
// core/scripture/ScriptureLibrary.h for exactly why these four, and
// why NIV/HCSB specifically are NOT here: both are actively
// copyrighted, and OpenLP has documented hitting the same "Bible
// societies require exorbitant licensing costs" wall trying to bundle
// them). The list is single-select, like EasyWorship's Resource
// Library -- click a translation to make it the active one; the search
// box, table, and preview all then operate against just that
// translation. "More Available..." stays as a real, honest link to a
// future Store for anything that needs an actual paid licence.
//
// SEARCH has two distinct modes (Words / Reference), based on direct
// research into how EasyWorship and OpenLP each do this (see the git
// history/design notes for sources) rather than a guess:
//
//   Words mode filters the verse table down to every verse whose text
//   contains all the typed words (case-insensitive, AND'd together) --
//   OpenLP calls this "Text" search, EasyWorship calls it "contextual"
//   search. A plain "search the Bible's contents" a la a search engine.
//
//   Reference mode is a progressively-constrained navigator, not a
//   filter, implementing the grammar documented in
//   core/scripture/ScriptureReference.h: Book -> Chapter -> Verse ->
//   optional end-Verse (optionally itself in a different chapter).
//   Both "John 3:16" (colon) and "John 3 16" (space) work identically,
//   matching EasyWorship's documented Quick Search flow of pressing
//   Spacebar between book/chapter/verse instead of typing punctuation.
//   While the book is still ambiguous ("J"), matching book names appear
//   as suggestion chips below the search box (Tab accepts the first
//   one); once resolved, a hint line reports the valid range for
//   whatever comes next ("John -- enter a chapter (1-21)") and rejects
//   anything outside it with a plain-language reason. Pressing
//   Enter/Return with only a book+chapter typed (no verse yet) jumps to
//   that chapter's verse 1, rather than doing nothing.
//
//   One EasyWorship behavior deliberately NOT copied: EasyWorship
//   toggles between these two modes via a single icon inside the search
//   box that silently changes appearance, which EasyWorship's own
//   support forum shows repeatedly confuses users ("I didn't know what
//   I did", "confusing to us all"). Two clearly-labeled, always-visible
//   buttons (below) make the current mode impossible to miss instead.
//
// SELECTION: the verse table allows BOTH a contiguous range (shift-
// click/shift-arrow/drag) AND Ctrl-click to add non-adjacent verses --
// matching EasyWorship's documented "hold Ctrl, click each verse"
// workflow for building a reading like Genesis 1:1,3,5. A single
// selected verse previews/adds as itself; multiple selected verses are
// compressed into ONE slide, combining their text and collapsing
// consecutive runs into ranges (e.g. "Esther 8:9-10; Esther 8:12")
// rather than sending each verse to Schedule separately -- see
// ScriptureTableModel::combinedReferenceAndText() in the .cpp for the
// exact grouping rule.
//
// Like MediaLibraryPanel, this panel does NOT render its own preview --
// the "how would this actually look" rendering lives one level up, in
// OperatorWindow's Item Preview panel (next to History). This panel's
// only job is to tell OperatorWindow *what* to preview, via
// previewRequested(), and what to add to Schedule, via
// scriptureActivated().
//
// The verse table is backed by ScriptureTableModel (a QAbstractTableModel
// over ScriptureLibrary::verses(translationCode), defined in this .cpp)
// rather than a QTableWidget populated with 31,000+ QTableWidgetItems --
// with a translation's full text loaded, a real model/view split is
// what keeps scrolling and filtering responsive.
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

    // Emitted when the current search mode's results become empty (Words
    // mode filtering out everything the operator typed) -- OperatorWindow
    // resets Item Preview to its default placeholder rather than leaving
    // whatever was previewed before the search stale and misleading, now
    // paired with a table that has visibly nothing selected.
    void previewCleared();

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
    void setActiveTranslation(const QString &code);
    void onSearchTextChanged(const QString &text);
    void updateWordsMode(const QString &text);
    void updateReferenceMode(const QString &text);
    void updateHintLabel(const ScriptureReference::Parsed &parsed);
    void rebuildSuggestionChips(const QVector<ScriptureReference::BookSuggestion> &suggestions);
    void applySelection(int firstSourceRow, int lastSourceRow);
    void onResetClicked();
    void onTranslationRowChanged();
    void onMoreAvailableClicked();
    void emitForSelection(bool activate);
    QVector<int> selectedSourceRowsSorted() const;
    void refreshReferenceCount();
    bool eventFilter(QObject *watched, QEvent *event) override;

    SearchMode m_mode = SearchMode::Reference;
    QString m_activeTranslation; // e.g. "KJV" -- see core/scripture/ScriptureLibrary::availableTranslations()

    QToolButton *m_wordsModeButton = nullptr;
    QToolButton *m_referenceModeButton = nullptr;
    QLineEdit *m_searchEdit = nullptr;
    QToolButton *m_resetButton = nullptr;

    QLabel *m_hintLabel = nullptr;
    QWidget *m_suggestionsRow = nullptr;
    QHBoxLayout *m_suggestionsLayout = nullptr;
    QString m_firstSuggestionCompletion; // what Tab accepts, if anything is currently suggested

    QListWidget *m_translationsList = nullptr;
    QPushButton *m_moreAvailableButton = nullptr;

    QTableView *m_table = nullptr;
    ScriptureTableModel *m_model = nullptr;
    QLabel *m_referenceCountLabel = nullptr;
};
