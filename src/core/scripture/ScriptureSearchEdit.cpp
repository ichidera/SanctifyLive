#include "ScriptureSearchEdit.h"

#include <QAction>
#include <QActionGroup>
#include <QCompleter>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QRegularExpression>
#include <QStringListModel>
#include <QTimer>

#include "../common/IconFactory.h"

ScriptureSearchEdit::ScriptureSearchEdit(QWidget *parent) : QLineEdit(parent)
{
    setPlaceholderText(tr("Search or type a reference, e.g. John 3:16"));

    m_completerModel = new QStringListModel(this);
    m_completer = new QCompleter(m_completerModel, this);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_completer->setCompletionMode(QCompleter::PopupCompletion);
    setCompleter(m_completer);

    // Both actions live at the leading position; only one is ever
    // visible, so swapping them is how the box flips between "plain
    // search field" (magnifying glass) and "committed reference"
    // (hamburger menu) display -- see applyIdleStyle()/applyCommittedStyle().
    m_leadingAction = addAction(IconFactory::search(14), QLineEdit::LeadingPosition);
    m_menuIconAction = addAction(IconFactory::hamburgerMenu(14), QLineEdit::LeadingPosition);
    m_menuIconAction->setVisible(false);

    connect(m_menuIconAction, &QAction::triggered, this, [this]() {
        QMenu menu(this);

        QMenu *viewMenu = menu.addMenu(tr("View"));
        QAction *singleLine = viewMenu->addAction(tr("Single Line View"));
        QAction *wordWrap = viewMenu->addAction(tr("Word Wrap View"));
        singleLine->setCheckable(true);
        wordWrap->setCheckable(true);
        wordWrap->setChecked(true);
        auto *viewGroup = new QActionGroup(&menu);
        viewGroup->addAction(singleLine);
        viewGroup->addAction(wordWrap);
        connect(wordWrap, &QAction::toggled, this, [this](bool on) {
            if (on)
                emit wordWrapToggleRequested(true);
        });
        connect(singleLine, &QAction::toggled, this, [this](bool on) {
            if (on)
                emit wordWrapToggleRequested(false);
        });

        QMenu *sortMenu = menu.addMenu(tr("Sort by"));
        QAction *sortAsc = sortMenu->addAction(tr("Ascending"));
        QAction *sortDesc = sortMenu->addAction(tr("Descending"));
        sortAsc->setCheckable(true);
        sortDesc->setCheckable(true);
        sortAsc->setChecked(true);
        auto *sortGroup = new QActionGroup(&menu);
        sortGroup->addAction(sortAsc);
        sortGroup->addAction(sortDesc);
        connect(sortAsc, &QAction::toggled, this, [this](bool on) {
            if (on)
                emit sortOrderChangeRequested(Qt::AscendingOrder);
        });
        connect(sortDesc, &QAction::toggled, this, [this](bool on) {
            if (on)
                emit sortOrderChangeRequested(Qt::DescendingOrder);
        });

        menu.addSeparator();
        connect(menu.addAction(tr("Refresh")), &QAction::triggered, this, &ScriptureSearchEdit::refreshRequested);

        menu.exec(mapToGlobal(QPoint(4, height())));
    });

    connect(this, &QLineEdit::textEdited, this, &ScriptureSearchEdit::handleTextEdited);

    applyIdleStyle();
}

void ScriptureSearchEdit::setLibrary(const BibleLibrary *library, const QString &translationCode)
{
    m_library = library;
    m_translationCode = translationCode;
}

void ScriptureSearchEdit::setTranslationCode(const QString &translationCode)
{
    m_translationCode = translationCode;
    // A book that was unambiguous in the old translation might not be in
    // the new one (or might now collide with another book), so re-derive
    // everything against the freshly-typed text.
    updateModeForText(text());
}

void ScriptureSearchEdit::commitReference(const QString &displayText)
{
    m_committed = true;
    m_lastAcceptedText.clear();
    setText(displayText);
    setReadOnly(true);
    setCursorPosition(0);
    applyCommittedStyle();
}

void ScriptureSearchEdit::focusInEvent(QFocusEvent *event)
{
    QLineEdit::focusInEvent(event);
    if (m_committed)
        revertToEditable();
}

void ScriptureSearchEdit::keyPressEvent(QKeyEvent *event)
{
    if (m_committed) {
        // Any key that actually edits the field (typing over it, or
        // Backspace/Delete) drops back to a plain editable search box
        // first; pure navigation (arrows, Tab, Home/End) leaves the
        // committed display alone.
        const bool isEditingKey = !event->text().isEmpty() || event->key() == Qt::Key_Backspace
            || event->key() == Qt::Key_Delete;
        if (isEditingKey)
            revertToEditable();
        else {
            QLineEdit::keyPressEvent(event);
            return;
        }
    }

    // Gate plain character insertion against the live reference parser
    // *before* it lands in the box, so a chapter/verse number that can
    // never be valid (e.g. chapter "9" in a 4-chapter book) is rejected
    // as it's typed rather than accepted and corrected after the fact.
    // Everything else (navigation, Backspace/Delete, shortcuts) passes
    // straight through -- removing characters can only relax
    // constraints, never violate them. Paste/undo/redo bypass this (no
    // single QKeyEvent to gate) and are instead caught after the fact in
    // handleTextEdited().
    const QString insertedText = event->text();
    const bool isPlainCharacterInsert = !insertedText.isEmpty() && insertedText.at(0).isPrint()
        && !(event->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier));

    if (isPlainCharacterInsert && m_library) {
        QString candidate = text();
        if (hasSelectedText())
            candidate.remove(selectionStart(), selectedText().length());
        const int insertAt = qBound(0, cursorPosition(), candidate.length());
        candidate.insert(insertAt, insertedText);

        if (!isStructurallyValid(candidate)) {
            flashInvalidFeedback();
            event->accept();
            return;
        }
    }

    QLineEdit::keyPressEvent(event);
}

void ScriptureSearchEdit::handleTextEdited(const QString &text)
{
    if (m_library && !isStructurallyValid(text)) {
        // Something bypassed the per-keystroke guard above (paste, IME
        // composition, undo/redo) and produced an impossible reference --
        // fall back to rejecting the whole edit rather than the letting
        // an unreachable chapter/verse sit in the box.
        const int pos = cursorPosition();
        setText(m_lastAcceptedText);
        setCursorPosition(qMin(pos, m_lastAcceptedText.length()));
        flashInvalidFeedback();
        return;
    }

    m_lastAcceptedText = text;
    updateModeForText(text);
}

ScriptureSearchEdit::LiveParse ScriptureSearchEdit::parseLive(const QString &text)
{
    // Mirrors BibleLibrary::parseReference's pattern, but every trailing
    // piece (book letters, chapter digits, colon, verse digits) is
    // allowed to be partial/absent so it can be evaluated after every
    // single keystroke, not just once the whole reference is complete.
    static const QRegularExpression pattern(
        QStringLiteral(R"(^\s*((?:[123]|i{1,3})\s+)?([A-Za-z][A-Za-z ]*?)?\s*(\d*)\s*(:)?\s*(\d*)\s*$)"),
        QRegularExpression::CaseInsensitiveOption);

    LiveParse result;
    const QRegularExpressionMatch match = pattern.match(text);
    if (!match.hasMatch())
        return result; // doesn't even look like the start of a reference

    result.bookPart = (match.captured(1) + match.captured(2)).trimmed();
    result.chapterPart = match.captured(3);
    result.hasColon = !match.captured(4).isEmpty();
    result.versePart = match.captured(5);
    return result;
}

bool ScriptureSearchEdit::isStructurallyValid(const QString &candidateText) const
{
    if (!m_library)
        return true;

    const LiveParse parsed = parseLive(candidateText);
    if (parsed.bookPart.isEmpty())
        return true; // nothing book-like typed yet -- can't be invalid

    const QStringList candidates = m_library->matchBookNames(m_translationCode, parsed.bookPart);
    if (candidates.size() != 1)
        return true; // not a book (word-search mode) or still ambiguous -- nothing concrete to check yet

    const QString &book = candidates.first();
    if (!parsed.chapterPart.isEmpty()) {
        const int chapterNum = parsed.chapterPart.toInt();
        if (!m_library->isValidChapter(m_translationCode, book, chapterNum))
            return false;

        if (parsed.hasColon && !parsed.versePart.isEmpty()) {
            const int verseNum = parsed.versePart.toInt();
            if (!m_library->isValidVerse(m_translationCode, book, chapterNum, verseNum))
                return false;
        }
    }
    return true;
}

void ScriptureSearchEdit::updateModeForText(const QString &text)
{
    if (!m_library)
        return;

    const LiveParse parsed = parseLive(text);
    if (parsed.bookPart.isEmpty()) {
        m_inReferenceMode = false;
        refreshCompleterModel({});
        emit ambiguousBookCandidates({});
        emit liveWordQuery(text);
        return;
    }

    const QStringList candidates = m_library->matchBookNames(m_translationCode, parsed.bookPart);
    if (candidates.isEmpty()) {
        // Doesn't match the start of any book -- word/sentence search mode.
        m_inReferenceMode = false;
        refreshCompleterModel({});
        emit ambiguousBookCandidates({});
        emit liveWordQuery(text);
        return;
    }

    m_inReferenceMode = true;
    emit liveWordQuery(QString()); // leaving word-search mode clears any stale results

    // Keep suggesting book-name completions only while the book itself
    // is still being typed; once a chapter number shows up the book is
    // settled and a completion popup would just be in the way.
    refreshCompleterModel(parsed.chapterPart.isEmpty() ? candidates : QStringList());
    emit ambiguousBookCandidates(candidates.size() > 1 ? candidates : QStringList());
}

void ScriptureSearchEdit::revertToEditable()
{
    m_committed = false;
    setReadOnly(false);
    clear();
    m_lastAcceptedText.clear();
    applyIdleStyle();
    emit editingResumed();
}

void ScriptureSearchEdit::refreshCompleterModel(const QStringList &candidates)
{
    m_completerModel->setStringList(candidates);
}

void ScriptureSearchEdit::applyIdleStyle()
{
    m_leadingAction->setVisible(true);
    m_menuIconAction->setVisible(false);
    setStyleSheet(QString());
}

void ScriptureSearchEdit::applyCommittedStyle()
{
    m_leadingAction->setVisible(false);
    m_menuIconAction->setVisible(true);
    setStyleSheet(QStringLiteral("QLineEdit { background-color: rgba(255, 255, 255, 18); }"));
}

void ScriptureSearchEdit::flashInvalidFeedback()
{
    setStyleSheet(QStringLiteral("QLineEdit { border: 1px solid #e74c3c; }"));
    QTimer::singleShot(150, this, [this]() {
        if (m_committed)
            applyCommittedStyle();
        else
            applyIdleStyle();
    });
}
