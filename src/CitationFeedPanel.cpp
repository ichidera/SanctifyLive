#include "CitationFeedPanel.h"
#include "Theme.h"

#include <QVBoxLayout>
#include <QListWidget>
#include <QLabel>

CitationFeedPanel::CitationFeedPanel(bool redVariant, QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_list = new QListWidget(this);
    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setWordWrap(true);
    m_list->setSelectionMode(QAbstractItemView::NoSelection);
    m_list->setSpacing(redVariant ? 2 : 6);
    layout->addWidget(m_list);

    if (redVariant) {
        m_list->setStyleSheet(QString("background-color:%1;").arg(Theme::kBgInset));
        populateRedVariant();
    } else {
        m_list->setStyleSheet(QString("background-color:%1;").arg(Theme::kBgPanel));
        populateCardVariant();
    }
}

void CitationFeedPanel::populateCardVariant()
{
    const QString ref = "Pslam 91:1[NLT] [v2]";
    const QString body =
        "He that dwelleth in the secret place of the most High shall abide "
        "under the shadow of the Almighty.";

    for (int i = 0; i < 4; ++i) {
        auto* item = new QListWidgetItem;
        auto* card = new QWidget;
        auto* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(10, 8, 10, 8);
        cardLayout->setSpacing(2);

        auto* refLabel = new QLabel(ref);
        refLabel->setStyleSheet(QString("color:%1; font-weight:700;")
                                     .arg(i == 0 ? Theme::kAccentGreen : Theme::kTextSecondary));
        auto* bodyLabel = new QLabel(body);
        bodyLabel->setWordWrap(true);

        cardLayout->addWidget(refLabel);
        cardLayout->addWidget(bodyLabel);

        item->setSizeHint(card->sizeHint());
        m_list->addItem(item);
        m_list->setItemWidget(item, card);
    }
}

void CitationFeedPanel::populateRedVariant()
{
    const QStringList verses = {
        "1 He that dwelleth in the secret place of the most High shall abide under the shadow of the Almighty.",
        "2 I will say of the LORD, He is my refuge and my fortress: my God; in him will I trust.",
        "3 Surely he shall deliver thee from the snare of the fowler, and from the noisome pestilence.",
        "4 He shall cover thee with his feathers, and under his wings shalt thou trust: his truth shall be thy shield and buckler.",
        "5 Thou shalt not be afraid for the terror by night; nor for the arrow that flieth by day;",
        "6 Nor for the pestilence that walketh in darkness; nor for the destruction that wasteth at noonday.",
    };
    for (const auto& v : verses) {
        auto* item = new QListWidgetItem(v);
        item->setForeground(QColor(Theme::kAccentRed));
        m_list->addItem(item);
    }
}
