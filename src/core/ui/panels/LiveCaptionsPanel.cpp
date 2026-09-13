#include "core/ui/panels/LiveCaptionsPanel.h"

#include <QLabel>
#include <QVBoxLayout>

LiveCaptionsPanel::LiveCaptionsPanel(QWidget *parent) : QWidget(parent)
{
    auto *title = new QLabel(tr("Live Captions"), this);
    title->setProperty("role", "sectionTitle");

    auto *status = new QLabel(tr("Not connected"), this);
    status->setProperty("role", "muted");

    auto *hint = new QLabel(
        tr("Speech-to-text transcription isn't wired up yet -- this panel "
           "is a placeholder for where it will appear once a live audio "
           "source is connected."),
        this);
    hint->setProperty("role", "muted");
    hint->setWordWrap(true);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(title);
    layout->addWidget(status);
    layout->addSpacing(8);
    layout->addWidget(hint);
    layout->addStretch(1);
}
