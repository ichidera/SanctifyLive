#ifndef SANCTIFYLIVE_CORE_SCRIPTURE_SCRIPTUREPANEL_H_
#define SANCTIFYLIVE_CORE_SCRIPTURE_SCRIPTUREPANEL_H_


#include <QWidget>

#include "BibleLibrary.h"
#include "../settings/OutputProfile.h"

class QListWidget;
class QListWidgetItem;
class QTreeWidget;
class QTreeWidgetItem;
class QLabel;
class QPushButton;
class QToolButton;
class QMenu;
class ScriptureSearchEdit;
class LiveAppearancePreview;

// ScripturePanel: the "Scriptures" content-type tab of the bottom
// resource library, laid out the way EasyWorship's Scripture panel
// works --
//
//   [ reference / keyword search box ]  [ Translation v ]  [ + ]
//   +--------------+  +--------------+  +---------------------------+
//   | Books        |  | Chapters     |  | Verses (or search results)|
//   | (OT/NT tree) |  | (number grid)|  | -- double-click sends live|
//   +--------------+  +--------------+  +---------------------------+
//   [ status / result-count footer ]
//
// The search box (ScriptureSearchEdit) does double duty, same as
// EasyWorship: type a reference like "John 3:16" and press Enter to
// jump straight there -- with book-name autocomplete and live
// chapter/verse-bounds validation as you type -- or type any other text
// to run a keyword search across the whole Bible in the selected
// translation. Selecting a book/chapter directly always clears any
// active search results.
//
// "Translation" is a dropdown of installed translations plus a "More
// Available..." entry (TranslationLibraryDialog) listing free,
// downloadable ones from the scrollmapper/bible_databases repository.
// The "+" button offers the same "Add Bible from disk..." import, plus
// simple folder/collection organizers for translations, mirroring
// EasyWorship's "+" menu.
//
// Actually sending something live is left to whoever owns this widget:
// on scriptureActivated(), the caller is expected to build a Slide and
// send it live (see OperatorWindow::onScriptureActivated), the same way
// MediaLibraryPanel::mediaActivated already works.
//
// This panel DOES own its live-appearance preview, though (the verse
// list's selection drives it) -- it uses the shared LiveAppearancePreview
// widget and the same composeProjectedScripture() helper that
// OperatorWindow::onScriptureActivated uses to build what actually gets
// projected, so the preview can never show different text than what
// "Send Selected" / double-click would actually put on screen. See
// src/core/scripture/ScriptureFormatting.h.
class ScripturePanel : public QWidget
{
    Q_OBJECT

public:
    explicit ScripturePanel(QWidget *parent = nullptr);
    ~ScripturePanel() override;

signals:
    // reference is a human-readable citation, e.g. "John 3:16" or
    // "Romans 8:28-30"; text is the verse (or joined verses) to project.
    void scriptureActivated(const QString &reference, const QString &text, const QString &translationCode);

public slots:
    // Which destination's resolution/margins/font this panel's preview
    // should represent. See LiveAppearancePreview::setProfile.
    void setOutputProfile(const OutputProfile &profile);

private slots:
    void onBookSelected(QTreeWidgetItem *item, int column);
    void onChapterSelected(QListWidgetItem *item);
    void onVerseItemActivated(QListWidgetItem *item);
    void onVerseSelectionChanged();
    void onSendSelectedClicked();
    void onSearchSubmitted();
    void onLiveWordQuery(const QString &text);
    void onWordWrapToggleRequested(bool wordWrap);
    void onSortOrderChangeRequested(Qt::SortOrder order);

private:
    void populateTranslations();
    void populateBookTree();
    void populateChapterGrid(const QString &bookName);
    void populateVerseList(const QString &bookName, int chapter);
    void showSearchResults(const QVector<BibleLibrary::SearchResult> &results, const QString &query);
    void jumpToReference(const BibleLibrary::ParsedReference &ref);
    void updateFooter();
    void updatePreview();
    // Given the verse list's current selection, resolves it to a single
    // book/chapter/verse-range using the same "if the selection isn't
    // all one book & chapter, just use the first item" rule
    // onSendSelectedClicked has always used for discontiguous
    // (search-result) selections. Returns false (selection empty) if
    // there's nothing to resolve. Shared by onSendSelectedClicked (the
    // real "go live" path) and updatePreview(), so both act on the
    // exact same interpretation of what's selected.
    bool resolveSelectionRange(QString *outBook, int *outChapter, int *outVerseStart, int *outVerseEnd) const;
    // Builds the reference string and the fully composed projected text
    // (verse body + "Reference (TRANSLATION)" footnote, via
    // composeProjectedScripture) for one book/chapter/verse-range.
    // Shared by sendVerses() and updatePreview() so the preview can
    // never show different text than what actually gets sent live.
    bool composeVerses(const QString &book, int chapter, int verseStart, int verseEnd, QString *outReference,
                        QString *outText) const;
    void sendVerses(const QString &book, int chapter, int verseStart, int verseEnd);
    QString currentTranslationCode() const;
    void selectTranslation(const QString &code);
    void rebuildTranslationMenu();
    void rebuildPlusMenu();
    void openTranslationLibraryDialog();
    void promptAddBibleFromDisk();
    void sortVerseList();

    BibleLibrary m_library;

    ScriptureSearchEdit *m_searchBox;
    QToolButton *m_translationButton;
    QMenu *m_translationMenu;
    QToolButton *m_plusButton;
    QMenu *m_plusMenu;
    QStringList m_customFolders; // simple named organizers created via the "+" menu

    QTreeWidget *m_bookTree;
    QListWidget *m_chapterGrid;
    QListWidget *m_verseList;
    QPushButton *m_sendSelectedButton;
    QLabel *m_footerLabel;
    LiveAppearancePreview *m_preview;

    QString m_currentTranslationCode;
    QString m_currentBook;
    int m_currentChapter = 0;
    bool m_searchResultsActive = false;
    bool m_libraryAvailable = false;
    Qt::SortOrder m_verseSortOrder = Qt::AscendingOrder;
    QString m_lastSearchQuery;
};

#endif // SANCTIFYLIVE_CORE_SCRIPTURE_SCRIPTUREPANEL_H_
