#include "HistoryBar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>

#include "ScheduleModel.h"

namespace {
constexpr int kSwatchSize = 32;

QIcon makeHistorySwatch(const QColor &color)
{
    QPixmap pixmap(kSwatchSize, kSwatchSize);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(color.darker(150), 1));
    painter.setBrush(color);
    painter.drawRoundedRect(QRectF(0.5, 0.5, kSwatchSize - 1, kSwatchSize - 1), 3, 3);
    return QIcon(pixmap);
}
}

HistoryBar::HistoryBar(ScheduleModel *model, QWidget *parent)
    : QWidget(parent), m_model(model)
{
    auto *header = new QLabel(tr("History"), this);
    header->setStyleSheet("color: #888; font-size: 11px; padding: 0 6px;");
    header->setFixedWidth(56);

    m_stripLayout = new QHBoxLayout();
    m_stripLayout->setContentsMargins(4, 2, 4, 2);
    m_stripLayout->setSpacing(4);
    m_stripLayout->addStretch(1);

    auto *stripWidget = new QWidget(this);
    stripWidget->setLayout(m_stripLayout);

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidget(stripWidget);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setFixedHeight(kSwatchSize + 16);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(header);
    layout->addWidget(scrollArea, 1);

    connect(m_model, &ScheduleModel::historyChanged, this, &HistoryBar::rebuild);
    rebuild();
}

void HistoryBar::rebuild()
{
    // Clear everything except the trailing stretch.
    while (m_stripLayout->count() > 1) {
        QLayoutItem *taken = m_stripLayout->takeAt(0);
        delete taken->widget();
        delete taken;
    }

    for (int i = 0; i < m_model->historyCount(); ++i) {
        const Slide &slide = m_model->historyAt(i);

        auto *button = new QPushButton(this);
        button->setIcon(makeHistorySwatch(slide.background));
        button->setIconSize(QSize(kSwatchSize, kSwatchSize));
        button->setText(slide.label);
        button->setToolTip(tr("Send \"%1\" live again").arg(slide.label));
        button->setStyleSheet(
            "QPushButton { background-color: #2b2b2e; border: 1px solid #3a3a3a; "
            "border-radius: 4px; padding: 2px 8px; color: #ddd; text-align: left; } "
            "QPushButton:hover { background-color: #35363c; }");

        connect(button, &QPushButton::clicked, this, [this, i]() {
            m_model->goLiveFromHistoryAt(i);
        });

        m_stripLayout->insertWidget(m_stripLayout->count() - 1, button);
    }

    if (m_model->historyCount() == 0) {
        auto *emptyLabel = new QLabel(tr("Nothing shown yet"), this);
        emptyLabel->setStyleSheet("color: #666; padding: 4px;");
        m_stripLayout->insertWidget(0, emptyLabel);
    }
}