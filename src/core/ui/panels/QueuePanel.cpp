#include "core/ui/panels/QueuePanel.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

#include "core/model/ScheduleModel.h"
#include "core/ui/Theme.h"

QueuePanel::QueuePanel(ScheduleModel *model, QWidget *parent)
    : QWidget(parent), m_model(model)
{
    auto *title = new QLabel(tr("Queue"), this);
    title->setProperty("role", "sectionTitle");

    m_list = new QListWidget(this);
    m_list->setAlternatingRowColors(false);
    m_list->installEventFilter(this);

    connect(m_list, &QListWidget::itemClicked, this, &QueuePanel::onItemClicked);
    connect(m_list, &QListWidget::itemDoubleClicked, this, &QueuePanel::onItemDoubleClicked);

    m_addButton = new QPushButton(tr("+ Add Slide"), this);
    m_removeButton = new QPushButton(tr("Remove"), this);
    connect(m_addButton, &QPushButton::clicked, this, &QueuePanel::addSlideRequested);
    connect(m_removeButton, &QPushButton::clicked, this, &QueuePanel::onRemoveClicked);

    auto *buttons = new QHBoxLayout();
    buttons->addWidget(m_addButton);
    buttons->addWidget(m_removeButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(title);
    layout->addWidget(m_list, /*stretch=*/1);
    layout->addLayout(buttons);

    connect(m_model, &ScheduleModel::scheduleChanged, this, &QueuePanel::onScheduleChanged);
    connect(m_model, &ScheduleModel::liveContentChanged, this, &QueuePanel::onCursorsChanged);
    connect(m_model, &ScheduleModel::previewChanged, this, &QueuePanel::onCursorsChanged);

    rebuild();
}

bool QueuePanel::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_list && event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            if (QListWidgetItem *item = m_list->currentItem()) {
                onItemDoubleClicked(item); // Enter commits to live, same as a double-click
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void QueuePanel::onScheduleChanged()
{
    rebuild();
}

void QueuePanel::onCursorsChanged()
{
    refreshHighlights();
}

void QueuePanel::onItemClicked(QListWidgetItem *item)
{
    m_model->setPreviewIndex(m_list->row(item));
}

void QueuePanel::onItemDoubleClicked(QListWidgetItem *item)
{
    m_model->setPreviewIndex(m_list->row(item));
    m_model->goLive();
}

void QueuePanel::onRemoveClicked()
{
    const int row = m_list->currentRow();
    if (row >= 0)
        m_model->removeSlideAt(row);
}

void QueuePanel::rebuild()
{
    QSignalBlocker blocker(m_list);
    m_list->clear();

    for (int i = 0; i < m_model->count(); ++i) {
        const Slide &slide = m_model->slideAt(i);
        // First line is the reference/label (bold in the design), second
        // line a short preview of the actual body text.
        QString preview = slide.text;
        preview.replace('\n', ' ');
        if (preview.size() > 90)
            preview = preview.left(90) + QStringLiteral("...");

        auto *item = new QListWidgetItem(slide.label + "\n" + preview);
        m_list->addItem(item);
    }

    refreshHighlights();
}

void QueuePanel::refreshHighlights()
{
    for (int i = 0; i < m_list->count(); ++i) {
        QListWidgetItem *item = m_list->item(i);

        const bool isLive = (i == m_model->currentIndex());
        const bool isPreview = (i == m_model->previewIndex());

        if (isLive) {
            item->setBackground(QColor(Theme::Colors::accentLive).lighter(150));
            item->setForeground(Qt::black);
        } else if (isPreview) {
            item->setBackground(QColor(Theme::Colors::accentSelect));
            item->setForeground(Qt::white);
        } else {
            item->setBackground(QColor(Theme::Colors::panelAlt));
            item->setForeground(QColor(Theme::Colors::textPrimary));
        }
    }
}
