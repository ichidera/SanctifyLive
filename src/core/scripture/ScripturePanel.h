#ifndef SANCTIFYLIVE_CORE_SCRIPTURE_SCRIPTUREPANEL_H_
#define SANCTIFYLIVE_CORE_SCRIPTURE_SCRIPTUREPANEL_H_


#include <QWidget>

#include "BibleLibrary.h"

class QComboBox;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QTreeWidget;
class QTreeWidgetItem;
class QLabel;
class QPushButton;
class QToolButton;

// ScripturePanel: the "Scriptures" content-type tab of the bottom
// resource library, laid out the way EasyWorship's Scripture panel
// works --
//
//   [ reference / keyword search box ]  [ translation dropdown ]
//   +--------------+  +--------------+  +---------------------------+
//   | Books        |  | Chapters     |  | Verses (or search results)|
//   | (OT/NT tree) |  | (number grid)|  | -- double-click sends live|
//   +--------------+  +--------------+  +---------------------------+
//   [ status / result-count footer ]
//
// The search box does double duty, same as EasyWorship: type a
// reference like "John 3:16" and press Enter to jump straight there,
// or type any other text to run a keyword search across the whole
// Bible in the selected translation. Selecting a book/chapter directly
// always clears any active search results.
//
// Actually showing something live is left to whoever owns this widget:
// on scriptureActivated(), the caller is expected to build a Slide and
// send it live (see OperatorWindow::onScriptureActivated), the same way
// MediaLibraryPanel::mediaActivated already works.
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

private slots:
    void onTranslationChanged(int index);
    void onBookSelected(QTreeWidgetItem *item, int column);
    void onChapterSelected(QListWidgetItem *item);
    void onVerseItemActivated(QListWidgetItem *item);
    void onSendSelectedClicked();
    void onSearchSubmitted();
    void onSearchTextEdited(const QString &text);

private:
    void populateTranslations();
    void populateBookTree();
    void populateChapterGrid(const QString &bookName);
    void populateVerseList(const QString &bookName, int chapter);
    void showSearchResults(const QVector<BibleLibrary::SearchResult> &results, const QString &query);
    void jumpToReference(const BibleLibrary::ParsedReference &ref);
    void updateFooter();
    void sendVerses(const QString &book, int chapter, int verseStart, int verseEnd);
    QString currentTranslationCode() const;

    BibleLibrary m_library;

    QLineEdit *m_searchBox;
    QComboBox *m_translationCombo;
    QTreeWidget *m_bookTree;
    QListWidget *m_chapterGrid;
    QListWidget *m_verseList;
    QPushButton *m_sendSelectedButton;
    QLabel *m_footerLabel;

    QString m_currentBook;
    int m_currentChapter = 0;
    bool m_searchResultsActive = false;
    bool m_libraryAvailable = false;
};

#endif // SANCTIFYLIVE_CORE_SCRIPTURE_SCRIPTUREPANEL_H_
