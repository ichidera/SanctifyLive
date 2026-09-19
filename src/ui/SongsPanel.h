#pragma once

#include <QVector>
#include <QWidget>

#include "core/song/Song.h"

class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QTableWidget;
class QToolButton;

// SongsPanel is the SONGS tab (one of the Content Tabs -- see
// OperatorWindow's panel glossary): a song list on the left (title,
// author, searchable), and the selected song's verse/chorus breakdown
// on the right -- the same "browse a library, see the current item's
// structure" shape as ScripturePanel's translation list + verse table,
// deliberately, since an operator who has learned one tab's layout
// shouldn't have to re-learn it for the other.
//
// DATA: backed by core/song/SongLibrary (persisted catalog, mirroring
// core/storage/MediaLibraryStore's shape) and core/song/Song (the
// OpenLyrics-style verse/chorus/bridge data model -- see that header
// for why). On first run, with no saved catalog yet, the library is
// seeded with a handful of public-domain hymns (SongLibrary::sampleSongs())
// so this tab has real, working content immediately rather than an
// empty list -- the same reasoning the Scriptures tab ships real Bible
// text instead of a placeholder.
//
// TWO GRANULARITIES of adding to Schedule, matching how songs actually
// get used live:
//   - Double-clicking (or pressing Enter on) a SONG in the left list
//     adds its ENTIRE running order (Song::orderedSections(), which
//     already expands a repeated chorus to however many times
//     verseOrder actually calls for it) as a sequence of slides in one
//     action -- see wholeSongActivated().
//   - Double-clicking a single SECTION in the right-hand breakdown adds
//     just that one section -- see sectionActivated(). Useful for
//     dropping in a single verse without the rest of the song, or
//     re-adding one section the worship leader is repeating live.
// A single click on either previews without adding anything, same
// click/double-click convention as MediaLibraryPanel and ScripturePanel.
//
// Like those two panels, SongsPanel does not render its own preview --
// OperatorWindow's Item Preview panel does that from whatever this
// panel emits.
class SongsPanel : public QWidget
{
    Q_OBJECT

public:
    explicit SongsPanel(QWidget *parent = nullptr);

signals:
    // A single section, added as one slide. `label` is e.g. "Amazing
    // Grace \u2014 Verse 1" (shown as the slide's label in Schedule/History).
    void sectionActivated(const QString &label, const QString &text);

    // Preview of whatever's currently selected (a section, or -- if a
    // song itself is selected/highlighted with no section chosen yet --
    // its first section) -- mirrors MediaLibraryPanel::previewRequested().
    void previewRequested(const QString &label, const QString &text);

    // A whole song's running order, added as a sequence of slides in one
    // action. `labels` and `texts` are parallel lists (same length, same
    // order slides should be added in) rather than a single compound
    // type, so OperatorWindow's slot can stay a plain loop over
    // ScheduleModel::addSlide() without needing a shared struct just for
    // this one signal.
    void wholeSongActivated(const QStringList &labels, const QStringList &texts);

private:
    QWidget *buildSongListColumn();
    QWidget *buildBreakdownArea();
    QWidget *buildBottomBar();

    void loadLibrary();
    void persistLibrary();
    void refreshSongList();
    void refreshBreakdownForCurrentSong();
    void onSongSelectionChanged();
    void onSongActivated(QListWidgetItem *item);
    void onSectionClicked(int row, int column);
    void onSectionActivated(int row, int column);
    void onSearchTextChanged(const QString &text);
    void onAddSongClicked();
    void onEditSongClicked();
    void onDeleteSongClicked();
    const Song *currentSong() const;

    QLineEdit *m_searchEdit = nullptr;
    QListWidget *m_songList = nullptr; // items' Qt::UserRole holds the song's id
    QPushButton *m_addButton = nullptr;
    QPushButton *m_editButton = nullptr;
    QPushButton *m_deleteButton = nullptr;

    QLabel *m_songTitleLabel = nullptr;
    QTableWidget *m_breakdownTable = nullptr;

    QLabel *m_countLabel = nullptr;

    QVector<Song> m_songs; // the full library; m_songList shows a filtered view of this
};
