#include "ui/SongEditorDialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QVBoxLayout>

#include "core/song/SongLibrary.h"
#include "core/song/SongTextFormat.h"
#include "ui/Theme.h"

SongEditorDialog::SongEditorDialog(QWidget *parent, const Song *existing) : QDialog(parent)
{
    setWindowTitle(existing ? tr("Edit Song") : tr("Add Song"));
    setMinimumSize(480, 520);

    auto *layout = new QVBoxLayout(this);

    layout->addWidget(new QLabel(tr("Title:"), this));
    m_titleEdit = new QLineEdit(this);
    layout->addWidget(m_titleEdit);

    layout->addWidget(new QLabel(tr("Author (optional):"), this));
    m_authorEdit = new QLineEdit(this);
    layout->addWidget(m_authorEdit);

    auto *lyricsHint = new QLabel(
        tr("Lyrics \u2014 tag each section on its own line, e.g. \u201c[Verse 1]\u201d or \u201c[Chorus]\u201d, "
           "with a blank line between sections:"),
        this);
    lyricsHint->setWordWrap(true);
    lyricsHint->setObjectName("nextSlideLabel");
    layout->addWidget(lyricsHint);

    m_lyricsEdit = new QPlainTextEdit(this);
    m_lyricsEdit->setPlaceholderText(tr("[Verse 1]\nAmazing grace, how sweet the sound\n"
                                        "That saved a wretch like me\n\n[Chorus]\nIt is well, with my soul\u2026"));
    QFont monoFont = m_lyricsEdit->font();
    monoFont.setFamily(QStringLiteral("monospace"));
    m_lyricsEdit->setFont(monoFont);
    layout->addWidget(m_lyricsEdit, /*stretch=*/1);

    auto *orderHint = new QLabel(
        tr("Verse order (optional) \u2014 space-separated section tags, e.g. \u201cv1 c1 v2 c1 v3 c1\u201d to repeat "
           "the chorus after each verse. Leave blank to just play each section once, in the order typed above."),
        this);
    orderHint->setWordWrap(true);
    orderHint->setObjectName("nextSlideLabel");
    layout->addWidget(orderHint);

    m_verseOrderEdit = new QLineEdit(this);
    m_verseOrderEdit->setPlaceholderText(tr("e.g. v1 c1 v2 c1 v3 c1"));
    layout->addWidget(m_verseOrderEdit);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setWordWrap(true);
    m_errorLabel->setStyleSheet(QStringLiteral("color: %1;").arg(Theme::kAccentRed));
    m_errorLabel->setVisible(false);
    layout->addWidget(m_errorLabel);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttonBox);

    if (existing) {
        m_existingId = existing->id;
        m_titleEdit->setText(existing->title);
        m_authorEdit->setText(existing->author);
        m_lyricsEdit->setPlainText(SongTextFormat::render(existing->sections));
        m_verseOrderEdit->setText(existing->verseOrder.join(QLatin1Char(' ')));
    }

    connect(buttonBox, &QDialogButtonBox::accepted, this, &SongEditorDialog::onAccept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &SongEditorDialog::reject);
}

void SongEditorDialog::onAccept()
{
    const QString title = m_titleEdit->text().trimmed();
    if (title.isEmpty()) {
        m_errorLabel->setText(tr("Please enter a title."));
        m_errorLabel->setVisible(true);
        m_titleEdit->setFocus();
        return;
    }

    QVector<SongSection> sections;
    if (!SongTextFormat::parse(m_lyricsEdit->toPlainText(), &sections)) {
        m_errorLabel->setText(
            tr("Couldn't find any tagged sections. Start a line with e.g. \u201c[Verse 1]\u201d before the lyrics."));
        m_errorLabel->setVisible(true);
        m_lyricsEdit->setFocus();
        return;
    }

    Song song;
    song.id = m_existingId.isEmpty() ? SongLibrary::newSongId() : m_existingId;
    song.title = title;
    song.author = m_authorEdit->text().trimmed();
    song.sections = sections;
    song.verseOrder = m_verseOrderEdit->text().split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);

    m_result = song;
    accept();
}
