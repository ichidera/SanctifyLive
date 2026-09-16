#include "ui/LiveCaptionsPanel.h"

#include <QFrame>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QVBoxLayout>

LiveCaptionsPanel::LiveCaptionsPanel(QWidget *parent) : QWidget(parent)
{
    m_view = new QPlainTextEdit(this);
    m_view->setObjectName("liveCaptionsView");
    m_view->setReadOnly(true);
    m_view->setFrameShape(QFrame::NoFrame);
    m_view->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    m_view->setPlaceholderText(tr("Waiting for live transcription\u2026"));
    // Selectable (an operator may want to copy a line out mid-service)
    // but not editable -- this is a display, not a text box, even
    // though QPlainTextEdit is what gives it scrolling/wrapping for
    // free.
    m_view->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_view);
}

void LiveCaptionsPanel::appendCaption(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty())
        return; // defensive no-op -- see the doc comment on this slot

    m_view->appendPlainText(trimmed);

    // Trim from the top once we're over the cap rather than letting the
    // document grow for the entire length of a service -- see
    // kMaxLines' doc comment in the header.
    QTextDocument *doc = m_view->document();
    while (doc->blockCount() > kMaxLines) {
        QTextCursor cursor(doc->firstBlock());
        cursor.select(QTextCursor::BlockUnderCursor);
        cursor.removeSelectedText();
        cursor.deleteChar(); // eat the now-empty line left behind by removeSelectedText()
    }

    // appendPlainText() moves the *document's* end, but the viewport's
    // scroll position doesn't necessarily follow -- if the operator had
    // scrolled up to reread an earlier line, the new caption would
    // arrive off-screen. Explicitly snap to the bottom every time so
    // the newest line is always what's visible, matching how live
    // captions/subtitles behave everywhere else.
    QScrollBar *scrollBar = m_view->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void LiveCaptionsPanel::clear()
{
    m_view->clear();
}
