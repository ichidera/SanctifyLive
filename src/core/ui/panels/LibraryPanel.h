#pragma once

#include <QString>
#include <QWidget>

class QListWidget;
class QListWidgetItem;

// LibraryPanel is the tabbed content browser along the bottom of the
// operator window: Songs / Scriptures / Media / Presentations / Themes.
//
// Only the Media tab is backed by something real right now -- it browses
// an actual "media/<category>" folder next to the executable, so images
// dropped there genuinely show up as thumbnails. The other tabs have no
// backing database yet (no song library, no Scripture text, no theme
// engine), so rather than fake rows of data, they show an honest "not
// built yet" placeholder. See the README roadmap for what unlocks each
// one -- when a real backend lands, only that tab's build*Tab() method
// needs to change; the tab structure itself doesn't.
class LibraryPanel : public QWidget
{
    Q_OBJECT

public:
    explicit LibraryPanel(QWidget *parent = nullptr);

private slots:
    void onMediaFolderChanged(QListWidgetItem *current, QListWidgetItem *previous);

private:
    QWidget *buildMediaTab();
    QWidget *buildPlaceholderTab(const QString &description);
    void populateMediaFiles(const QString &categoryName);

    QListWidget *m_mediaFolders = nullptr;
    QListWidget *m_mediaFiles = nullptr;
    QString m_mediaRoot; // e.g. <app dir>/media
};
