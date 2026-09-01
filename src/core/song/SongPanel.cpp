#include "SongPanel.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QSplitter>
#include <QToolButton>
#include <QVBoxLayout>

#include "../output/LiveAppearancePreview.h"
#include "../schedule/Slide.h"
#include "SongEditorDialog.h"

SongPanel::SongPanel(QWidget *parent) : QWidget(parent)
{
    populateSampleSongs();

    m_songList = new QListWidget(this);
    connect(m_songList, &QListWidget::currentItemChanged, this, &SongPanel::onSongSelectionChanged);
    m_songList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_songList, &QListWidget::customContextMenuRequested, this, &SongPanel::onSongContextMenu);

    m_addButton = new QToolButton(this);
    m_addButton->setText(QStringLiteral("+"));
    m_addButton->setToolTip(tr("Add a song"));
    connect(m_addButton, &QToolButton::clicked, this, &SongPanel::onAddSongClicked);

    auto *songListFooter = new QHBoxLayout();
    songListFooter->addWidget(m_addButton);
    songListFooter->addStretch(1);

    auto *songListPanel = new QWidget(this);
    auto *songListPanelLayout = new QVBoxLayout(songListPanel);
    songListPanelLayout->setContentsMargins(0, 0, 0, 0);
    songListPanelLayout->addWidget(new QLabel(tr("Songs"), songListPanel));
    songListPanelLayout->addWidget(m_songList, 1);
    songListPanelLayout->addLayout(songListFooter);

    m_slideList = new QListWidget(this);
    m_slideList->setWordWrap(true);
    connect(m_slideList, &QListWidget::currentItemChanged, this, &SongPanel::onSlideSelectionChanged);
    connect(m_slideList, &QListWidget::itemActivated, this, &SongPanel::onSlideActivated);
    connect(m_slideList, &QListWidget::itemDoubleClicked, this, &SongPanel::onSlideActivated);

    m_itemCountLabel = new QLabel(this);
    m_itemCountLabel->setStyleSheet("color: #888; padding: 2px 6px;");

    auto *slidePanel = new QWidget(this);
    auto *slidePanelLayout = new QVBoxLayout(slidePanel);
    slidePanelLayout->setContentsMargins(0, 0, 0, 0);
    slidePanelLayout->addWidget(new QLabel(tr("Slides \u2014 double-click to send live"), slidePanel));
    slidePanelLayout->addWidget(m_slideList, 1);
    slidePanelLayout->addWidget(m_itemCountLabel);

    m_preview = new LiveAppearancePreview(this);

    auto *splitter = new QSplitter(this);
    splitter->addWidget(songListPanel);
    splitter->addWidget(slidePanel);
    splitter->addWidget(m_preview);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);
    splitter->setStretchFactor(2, 3);

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(4, 4, 4, 4);
    rootLayout->addWidget(splitter, 1);

    rebuildSongList();
    if (m_songList->count() > 0)
        m_songList->setCurrentRow(0);
}

void SongPanel::populateSampleSongs()
{
    // A single public-domain hymn, purely to prove the layout and the
    // live-appearance preview -- exactly the role MediaLibraryPanel's
    // built-in color swatches play for Media.
    m_songs = {
        {tr("Amazing Grace"),
         tr("Amazing grace, how sweet the sound\n"
            "That saved a wretch like me\n"
            "I once was lost, but now am found\n"
            "Was blind, but now I see\n"
            "\n"
            "'Twas grace that taught my heart to fear\n"
            "And grace my fears relieved\n"
            "How precious did that grace appear\n"
            "The hour I first believed")},
    };
}

void SongPanel::rebuildSongList()
{
    m_songList->clear();
    for (int i = 0; i < m_songs.size(); ++i) {
        auto *item = new QListWidgetItem(m_songs.at(i).title, m_songList);
        item->setData(Qt::UserRole, i);
    }
}

QStringList SongPanel::composeSlides(const SongEntry &song) const
{
    // Split lyrics into stanzas on blank lines, then further split any
    // stanza longer than the configured max into consecutive chunks --
    // same idea as most projection software's "lines per slide" setting.
    QStringList currentStanza;
    QVector<QStringList> stanzas;
    for (const QString &line : song.lyrics.split('\n')) {
        if (line.trimmed().isEmpty()) {
            if (!currentStanza.isEmpty()) {
                stanzas.append(currentStanza);
                currentStanza.clear();
            }
        } else {
            currentStanza.append(line);
        }
    }
    if (!currentStanza.isEmpty())
        stanzas.append(currentStanza);

    // 0 (or negative, shouldn't happen from the spin box but be safe)
    // means "don't split" -- one slide per stanza regardless of length.
    const int maxLines = m_profile.songMaxLinesPerSlide > 0 ? m_profile.songMaxLinesPerSlide : 9999;

    QStringList slides;
    for (const QStringList &stanza : stanzas) {
        for (int i = 0; i < stanza.size(); i += maxLines) {
            QString text = stanza.mid(i, maxLines).join(QLatin1Char('\n'));
            if (m_profile.songShowSongTitle)
                text = song.title + QStringLiteral("\n\n") + text;
            slides.append(text);
        }
    }
    return slides;
}

void SongPanel::setOutputProfile(const OutputProfile &profile)
{
    m_profile = profile;
    m_preview->setProfile(profile);
    // Max-lines-per-slide and "show song title" both change what
    // composeSlides() produces, so the slide list itself (not just the
    // preview) has to be rebuilt when the profile changes.
    rebuildSlideListForCurrentSong();
}

void SongPanel::onSongSelectionChanged()
{
    rebuildSlideListForCurrentSong();
}

void SongPanel::rebuildSlideListForCurrentSong()
{
    m_slideList->clear();
    QListWidgetItem *songItem = m_songList->currentItem();
    if (!songItem) {
        m_itemCountLabel->clear();
        m_preview->clearSlide();
        return;
    }

    const int songIndex = songItem->data(Qt::UserRole).toInt();
    if (songIndex < 0 || songIndex >= m_songs.size()) {
        m_preview->clearSlide();
        return;
    }

    const QStringList slides = composeSlides(m_songs.at(songIndex));
    for (int i = 0; i < slides.size(); ++i) {
        const QString firstLine = slides.at(i).split('\n').value(0);
        auto *item = new QListWidgetItem(tr("%1. %2").arg(i + 1).arg(firstLine), m_slideList);
        item->setData(Qt::UserRole, slides.at(i));
    }
    m_itemCountLabel->setText(slides.size() == 1 ? tr("1 slide") : tr("%1 slides").arg(slides.size()));

    if (m_slideList->count() > 0)
        m_slideList->setCurrentRow(0);
    else
        m_preview->clearSlide();
}

void SongPanel::onSlideSelectionChanged()
{
    QListWidgetItem *item = m_slideList->currentItem();
    if (!item) {
        m_preview->clearSlide();
        return;
    }
    // Songs default to a plain black background for this first milestone
    // -- per-song/per-verse backgrounds are a natural follow-up once
    // Themes and Songs can be linked together.
    m_preview->setSlide(Slide(tr("Song Slide"), item->data(Qt::UserRole).toString(), Qt::black));
}

void SongPanel::onSlideActivated(QListWidgetItem *item)
{
    if (!item)
        return;
    QListWidgetItem *songItem = m_songList->currentItem();
    const QString title = songItem ? songItem->text() : tr("Song");
    emit songSlideActivated(title, item->data(Qt::UserRole).toString());
}

void SongPanel::onAddSongClicked()
{
    SongEditorDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    m_songs.append({dialog.title(), dialog.lyrics()});
    rebuildSongList();
    m_songList->setCurrentRow(m_songList->count() - 1);
}

void SongPanel::onSongContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = m_songList->itemAt(pos);
    if (!item)
        return;
    const int index = item->data(Qt::UserRole).toInt();
    if (index < 0 || index >= m_songs.size())
        return;

    QMenu menu(this);
    QAction *editAction = menu.addAction(tr("Edit..."));
    QAction *deleteAction = menu.addAction(tr("Delete"));
    QAction *chosen = menu.exec(m_songList->viewport()->mapToGlobal(pos));

    if (chosen == editAction) {
        SongEditorDialog dialog(this);
        dialog.setTitle(m_songs.at(index).title);
        dialog.setLyrics(m_songs.at(index).lyrics);
        if (dialog.exec() != QDialog::Accepted)
            return;
        m_songs[index] = {dialog.title(), dialog.lyrics()};
        rebuildSongList();
        m_songList->setCurrentRow(index);
    } else if (chosen == deleteAction) {
        if (QMessageBox::question(this, tr("Delete Song"),
                                   tr("Delete \u201c%1\u201d? This can't be undone.").arg(m_songs.at(index).title))
            != QMessageBox::Yes)
            return;
        m_songs.remove(index);
        rebuildSongList();
        if (m_songList->count() > 0)
            m_songList->setCurrentRow(qMin(index, m_songList->count() - 1));
    }
}
