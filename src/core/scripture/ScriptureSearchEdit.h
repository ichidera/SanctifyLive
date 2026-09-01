#ifndef SANCTIFYLIVE_CORE_SCRIPTURE_SCRIPTURESEARCHEDIT_H_
#define SANCTIFYLIVE_CORE_SCRIPTURE_SCRIPTURESEARCHEDIT_H_

#include <QLineEdit>

#include "BibleLibrary.h"

class QAction;
class QCompleter;
class QStringListModel;

// ScriptureSearchEdit is the "Search or type a reference, e.g. John
// 3:16" box at the top of ScripturePanel. It runs in one of two modes,
// decided fresh on every keystroke from the text alone (never something
// the person has to switch manually):
//
//  * Reference mode -- the text so far could be the start of a book
//    name ("g", "gen", "song of sol..."). While in this mode:
//      - a QCompleter suggests every book that matches what's typed so
//        far ("g" -> Genesis, Galatians; "ge" -> Genesis), narrowing as
//        more letters are typed, while still letting the person type
//        the full name by hand instead of picking a suggestion.
//      - once exactly one book is implied, a chapter number can follow
//        (a space, then digits); a verse can follow that (":" then
//        digits). Each digit is validated the instant it's typed
//        against BibleLibrary::isValidChapter()/isValidVerse() for the
//        resolved book -- a keystroke that would make the chapter or
//        verse number impossible (e.g. chapter "9" in a 4-chapter book)
//        is rejected outright rather than accepted and corrected later.
//
//  * Word/sentence search mode -- as soon as what's typed doesn't match
//    the start of any book name, the whole box is just a free-text
//    query: no more autocomplete or numeric validation, and
//    liveWordQuery() starts firing so the panel can run a keyword
//    search as the person types.
//
// Once Enter resolves a valid reference, the box switches to a
// read-only-styled "committed" display of that reference (a small menu
// button in place of the search icon, offering View / Sort by /
// Refresh -- see commitReference()). Clicking into the box or typing
// again immediately drops back to a normal editable search field.
class ScriptureSearchEdit : public QLineEdit
{
    Q_OBJECT

public:
    explicit ScriptureSearchEdit(QWidget *parent = nullptr);

    // Must be called once (and again whenever the translation changes)
    // so autocomplete/validation checks the right book list.
    void setLibrary(const BibleLibrary *library, const QString &translationCode);
    void setTranslationCode(const QString &translationCode);

    // Switches the box into the "committed reference" display described
    // above. Call this after a search successfully resolves.
    void commitReference(const QString &displayText);
    bool isCommitted() const { return m_committed; }

signals:
    // Fired (debounce-free; the panel can throttle if needed) whenever
    // the box is in word-search mode and the text changes. Empty text
    // means "clear search results".
    void liveWordQuery(const QString &query);

    // Fired whenever reference-mode autocomplete narrows to more than
    // one still-possible book, so the panel can show/hide a hint if it
    // wants to. Empty list = not currently ambiguous (either resolved
    // to one book, or not in reference mode at all).
    void ambiguousBookCandidates(const QStringList &candidates);

    // The three actions available from the "committed" state's menu
    // button (see commitReference()). The panel owns what they actually
    // do (word-wrap toggle, sort order, re-querying the verse list).
    void wordWrapToggleRequested(bool wordWrap);
    void sortOrderChangeRequested(Qt::SortOrder order);
    void refreshRequested();

    // The box reverted from committed back to editable (focused, or the
    // person started typing over the committed text).
    void editingResumed();

protected:
    void focusInEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void handleTextEdited(const QString &text);

private:
    struct LiveParse
    {
        QString bookPart;    // letters (+ leading ordinal) typed so far
        QString chapterPart; // digits typed for the chapter, if any
        QString versePart;   // digits typed for the verse, if any
        bool hasColon = false;
    };

    static LiveParse parseLive(const QString &text);
    // Returns false if `candidateText` is structurally invalid (a
    // chapter/verse number that can no longer be valid for the
    // currently-typed book) and should be rejected.
    bool isStructurallyValid(const QString &candidateText) const;
    void updateModeForText(const QString &text);
    void revertToEditable();
    void refreshCompleterModel(const QStringList &candidates);
    void applyIdleStyle();
    void applyCommittedStyle();
    void flashInvalidFeedback();

    const BibleLibrary *m_library = nullptr;
    QString m_translationCode;

    QCompleter *m_completer = nullptr;
    QStringListModel *m_completerModel = nullptr;

    QAction *m_leadingAction = nullptr; // search icon <-> hamburger menu, swapped in place
    QAction *m_menuIconAction = nullptr;

    bool m_committed = false;
    bool m_inReferenceMode = false;
    QString m_lastAcceptedText;
};

#endif // SANCTIFYLIVE_CORE_SCRIPTURE_SCRIPTURESEARCHEDIT_H_
