#pragma once

#include <QWidget>

class ScheduleModel;

// OutputWindow renders a slide from the ScheduleModel. It has two modes:
//
//   - Live:    tracks liveContentChanged(); this is the actual
//              congregation-facing output, and the small in-app mirror
//              of it.
//   - Preview: tracks previewChanged(); this is what the operator is
//              staging and has NOT been sent to the audience yet.
//
// Same rendering code either way -- the only difference is which model
// signal it listens to and which slide it asks for. Keeping this as one
// class (rather than two) guarantees the preview and live panes always
// look pixel-identical, which matters: the operator needs to trust that
// what they previewed is exactly what will go out.
//
// NOTE on architecture: still a plain QWidget with paintEvent for this
// milestone. See the earlier note in this file's history about moving
// to QOpenGLWidget/QRhi once media/video layers are introduced.
class OutputWindow : public QWidget
{
    Q_OBJECT

public:
    enum class Source { Live, Preview };

    explicit OutputWindow(ScheduleModel *model, Source source, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onContentChanged();

private:
    ScheduleModel *m_model;
    Source m_source;
};