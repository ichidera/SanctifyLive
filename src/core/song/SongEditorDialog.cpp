#include "SongEditorDialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QVBoxLayout>

SongEditorDialog::SongEditorDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Song"));
    resize(420, 380);

    m_titleEdit = new QLineEdit(this);
    m_titleEdit->setPlaceholderText(tr("Song title"));

    m_lyricsEdit = new QPlainTextEdit(this);
    m_lyricsEdit->setPlaceholderText(tr("Verse 1 line one\nVerse 1 line two\n\nChorus line one\nChorus line two"));

    m_hintLabel = new QLabel(
        tr("Separate verses/choruses with a blank line. Each one becomes its own slide (or several, if it's "
           "longer than the Song tab's \"Max lines per slide\" setting in Options)."),
        this);
    m_hintLabel->setWordWrap(true);
    m_hintLabel->setStyleSheet("color: #888;");

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &SongEditorDialog::onAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(tr("Title"), this));
    layout->addWidget(m_titleEdit);
    layout->addWidget(new QLabel(tr("Lyrics"), this));
    layout->addWidget(m_lyricsEdit, 1);
    layout->addWidget(m_hintLabel);
    layout->addWidget(buttons);
}

void SongEditorDialog::setTitle(const QString &title)
{
    m_titleEdit->setText(title);
}

void SongEditorDialog::setLyrics(const QString &lyrics)
{
    m_lyricsEdit->setPlainText(lyrics);
}

QString SongEditorDialog::title() const
{
    return m_titleEdit->text().trimmed();
}

QString SongEditorDialog::lyrics() const
{
    return m_lyricsEdit->toPlainText();
}

void SongEditorDialog::onAccept()
{
    if (title().isEmpty()) {
        QMessageBox::warning(this, tr("Song"), tr("Give the song a title before saving."));
        return;
    }
    if (lyrics().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Song"), tr("Add at least one line of lyrics before saving."));
        return;
    }
    accept();
}
