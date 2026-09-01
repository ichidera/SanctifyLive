#ifndef SANCTIFYLIVE_CORE_SONG_SONGPANEL_H_
#define SANCTIFYLIVE_CORE_SONG_SONGPANEL_H_

#include <QVector>
#include <QWidget>

#include "../settings/OutputProfile.h"

class QListWidget;
class QListWidgetItem;
class QToolButton;
class QLabel;
class LiveAppearancePreview;

// SongPanel: the "Songs" content-type tab of the bottom resource
// library. First-milestone scope, deliberately minimal (same spirit as
// Slide.h / ThemePanel.h): an in-memory list of songs, each just a title
// and raw lyrics text (verses/choruses separated by a blank line, typed
// in SongEditorDialog) -- no CCLI metadata, tags, or per-song
// backgrounds yet.
//
// The one thing this panel does carefully is turn lyrics into slides the
// *same way twice* -- once to populate the slide list + drive the live
// preview, and again (identically, because it's the same function) when
// something is actually sent live -- so what the operator previewed is
// guaranteed to be what goes out. See composeSlides(). That splitting
// respects OutputProfile's Song tab settings (Options > Main Output >
// Song): songMaxLinesPerSlide further breaks up a long stanza, and
// songShowSongTitle prepends the title as a header line on every slide.
class SongPanel : public QWidget
{
    Q_OBJECT

public:
    explicit SongPanel(QWidget *parent = nullptr);

signals:
    // songTitle is the operator-facing label (shown in the Schedule/
    // History, never on screen itself -- consistent with Slide::label
    // elsewhere); slideText is the fully composed text to project,
    // already broken to size and title-prefixed per the current profile.
    void songSlideActivated(const QString &songTitle, const QString &slideText);

public slots:
    void setOutputProfile(const OutputProfile &profile);

private slots:
    void onSongSelectionChanged();
    void onSlideSelectionChanged();
    void onSlideActivated(QListWidgetItem *item);
    void onAddSongClicked();
    void onSongContextMenu(const QPoint &pos);

private:
    struct SongEntry
    {
        QString title;
        QString lyrics; // raw text; blank lines separate stanzas
    };

    void populateSampleSongs();
    void rebuildSongList();
    void rebuildSlideListForCurrentSong();
    QStringList composeSlides(const SongEntry &song) const;

    QListWidget *m_songList;
    QListWidget *m_slideList;
    LiveAppearancePreview *m_preview;
    QToolButton *m_addButton;
    QLabel *m_itemCountLabel;

    QVector<SongEntry> m_songs;
    OutputProfile m_profile;
};

#endif // SANCTIFYLIVE_CORE_SONG_SONGPANEL_H_
