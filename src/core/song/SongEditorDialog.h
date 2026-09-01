#ifndef SANCTIFYLIVE_CORE_SONG_SONGEDITORDIALOG_H_
#define SANCTIFYLIVE_CORE_SONG_SONGEDITORDIALOG_H_

#include <QDialog>

class QLineEdit;
class QPlainTextEdit;
class QLabel;

// A deliberately simple add/edit form for SongPanel's in-memory song
// library: a title field and a plain-text lyrics box. Lyrics are typed
// as ordinary verses/choruses separated by a blank line -- SongPanel is
// the one that turns those stanzas (further split per
// OutputProfile::songMaxLinesPerSlide) into individual projectable
// slides, this dialog just collects the raw text.
class SongEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SongEditorDialog(QWidget *parent = nullptr);

    void setTitle(const QString &title);
    void setLyrics(const QString &lyrics);

    QString title() const;
    QString lyrics() const;

private slots:
    void onAccept();

private:
    QLineEdit *m_titleEdit;
    QPlainTextEdit *m_lyricsEdit;
    QLabel *m_hintLabel;
};

#endif // SANCTIFYLIVE_CORE_SONG_SONGEDITORDIALOG_H_
