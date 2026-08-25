#pragma once

#include <QWidget>

class ScheduleModel;

// OutputWindow is what actually gets projected/displayed to the
// congregation. It observes a ScheduleModel and repaints whenever the
// live content changes -- it never receives direct commands from the
// operator UI. Keeping this one-directional (model -> output) is what
// lets future control surfaces (web remote, stage view) drive the same
// output without this class needing to know they exist.
//
// NOTE on architecture: this is a plain QWidget with a paintEvent for
// milestone one, which is enough to prove the control loop end-to-end.
// Once media/video layers are introduced, this should become a
// QOpenGLWidget (or move to Qt Quick/QRhi) so slide composition is
// GPU-accelerated and can run its own render loop independent of the
// operator UI thread -- see the "Core render & output pipeline" step
// in the build plan. Swapping that in later shouldn't require changes
// outside this file, since everything else only talks to ScheduleModel.
class OutputWindow : public QWidget
{
    Q_OBJECT

public:
    explicit OutputWindow(ScheduleModel *model, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onLiveContentChanged();

private:
    ScheduleModel *m_model;
};
