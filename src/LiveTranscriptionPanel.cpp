#include "LiveTranscriptionPanel.h"
#include "Theme.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QVariant>

LiveTranscriptionPanel::LiveTranscriptionPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    auto* title = new QLabel("Live transcription");
    title->setProperty("role", QVariant("sectionTitle"));
    layout->addWidget(title);

    m_list = new QListWidget(this);
    m_list->setWordWrap(true);
    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setSelectionMode(QAbstractItemView::NoSelection);
    layout->addWidget(m_list, 1);

    populateSampleData();
}

void LiveTranscriptionPanel::populateSampleData()
{
    const QStringList verses = {
        "1 He that dwelleth in the secret place of the most High shall abide under the shadow of the Almighty.",
        "2 I will say of the LORD, He is my refuge and my fortress: my God; in him will I trust.",
        "3 Surely he shall deliver thee from the snare of the fowler, and from the noisome pestilence.",
        "4 He shall cover thee with his feathers, and under his wings shalt thou trust: his truth shall be thy shield and buckler.",
    };
    for (const auto& v : verses) {
        auto* item = new QListWidgetItem(v);
        item->setForeground(QColor(Theme::kAccentGreen));
        m_list->addItem(item);
    }
}
