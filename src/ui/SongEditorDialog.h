#pragma once

#include <QDialog>

#include "core/song/Song.h"

class QLineEdit;
class QPlainTextEdit;
class QLabel;

// SongEditorDialog is the "+ Add Song" / "Edit" editor for SongsPanel's
// library. Lyrics are entered as plain text using the bracket-tag
// convention documented in core/song/SongTextFormat.h ("[Verse 1]" /
// "[Chorus]" / etc, one section per bracket, blank line between
// sections) rather than a bespoke multi-field verse/chorus/bridge GUI --
// it's the same convention ChordPro/OpenSong-style tools use, so
// someone pasting in lyrics they already have typed up elsewhere is
// pasting into a format they may well already recognize, and it keeps
// this dialog to one text area instead of a dynamically-growing list of
// section editors.
//
// Verse order (which sections play in which sequence, e.g.
// "v1 c1 v2 c1 v3 c1" to repeat a chorus after each verse) is a
// separate field rather than something inferred from the lyrics text,
// since "sections in the order typed" and "sections in the order
// they're actually sung" are genuinely different things for any song
// with a repeating chorus.
class SongEditorDialog : public QDialog
{
    Q_OBJECT

public:
    // `existing`, if non-null, pre-fills every field for editing; the
    // dialog otherwise starts blank, ready for a new song.
    explicit SongEditorDialog(QWidget *parent = nullptr, const Song *existing = nullptr);

    // Valid only after exec() returns QDialog::Accepted. Preserves the
    // original song's id when editing; assigns a fresh one for a new
    // song.
    Song result() const { return m_result; }

private:
    void onAccept();

    QLineEdit *m_titleEdit = nullptr;
    QLineEdit *m_authorEdit = nullptr;
    QPlainTextEdit *m_lyricsEdit = nullptr;
    QLineEdit *m_verseOrderEdit = nullptr;
    QLabel *m_errorLabel = nullptr;

    QString m_existingId; // empty for a new song
    Song m_result;
};
