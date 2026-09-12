#include "QueuePanel.h"
#include "Theme.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QVariant>

QueuePanel::QueuePanel(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    auto* title = new QLabel("Queue");
    title->setProperty("role", QVariant("sectionTitle"));
    layout->addWidget(title);

    m_list = new QListWidget(this);
    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setWordWrap(true);
    m_list->setSpacing(4);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(m_list, 1);

    populateSampleData();
}

void QueuePanel::populateSampleData()
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
        card->setStyleSheet(QString(
            "background-color:%1; border-radius:4px;")
            .arg(i == 0 ? "#5c2a2a" : Theme::kBgPanelAlt));

        auto* refLabel = new QLabel(ref);
        refLabel->setStyleSheet("font-weight:700;");
        auto* bodyLabel = new QLabel(body);
        bodyLabel->setWordWrap(true);

        cardLayout->addWidget(refLabel);
        cardLayout->addWidget(bodyLabel);

        item->setSizeHint(card->sizeHint());
        m_list->addItem(item);
        m_list->setItemWidget(item, card);
    }
    m_list->setCurrentRow(0);
}
