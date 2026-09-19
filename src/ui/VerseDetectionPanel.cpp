#include "ui/VerseDetectionPanel.h"

#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>

VerseDetectionPanel::VerseDetectionPanel(QWidget *parent) : QWidget(parent)
{
    m_placeholderLabel = new QLabel(tr("No verses detected yet."), this);
    m_placeholderLabel->setObjectName("nextSlideLabel");
    m_placeholderLabel->setWordWrap(true);

    m_list = new QListWidget(this);
    m_list->setVisible(false);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    layout->addWidget(m_placeholderLabel);
    layout->addWidget(m_list, /*stretch=*/1);

    // Click = preview, double-click/Enter = add -- the same convention
    // used throughout the Content Tabs (MediaLibraryPanel,
    // ScripturePanel, SongsPanel), so a detected verse behaves exactly
    // like one found by hand in the Scriptures tab.
    connect(m_list, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        emit detectionPreviewRequested(item->data(Qt::UserRole).toString(),
                                        item->data(Qt::UserRole + 1).toString());
    });
    connect(m_list, &QListWidget::itemActivated, this, [this](QListWidgetItem *item) {
        emit detectionActivated(item->data(Qt::UserRole).toString(), item->data(Qt::UserRole + 1).toString());
    });
}

void VerseDetectionPanel::showDetections(const QVector<ScriptureDetector::Detection> &detections)
{
    m_list->clear();
    for (const ScriptureDetector::Detection &detection : detections) {
        auto *item = new QListWidgetItem(
            tr("%1 \u2014 %2").arg(detection.reference, detection.text.left(60)), m_list);
        item->setData(Qt::UserRole, detection.reference);
        item->setData(Qt::UserRole + 1, detection.text);
        item->setToolTip(detection.text);
        m_list->addItem(item);
    }
    m_placeholderLabel->setVisible(detections.isEmpty());
    m_list->setVisible(!detections.isEmpty());
}
