#pragma once

#include <QString>
#include <QVector>

#include "core/model/Slide.h"

// ScheduleIO turns a list of Slides into a file on disk and back again.
// This is deliberately kept OUT of ScheduleModel itself: the model's job
// is to be the live source of truth for "what is the service doing right
// now," not to know about file formats or QFile error handling. Keeping
// serialization here means ScheduleModel stays trivially testable and
// reusable (e.g. from a future headless/CLI mode) without dragging in
// file-format concerns, and the on-disk format can change without
// touching the model at all.
//
// The UI layer (OperatorWindow) is the only thing that talks to
// QFileDialog; it hands the chosen path to these functions and reports
// success/failure, exactly like it does with any other core/ call.
namespace ScheduleIO
{
// Serializes the given slides to a simple JSON document at `path`.
// Returns true on success; on failure `errorMessage` (if non-null) is
// set to a human-readable reason suitable for showing the operator.
bool saveToFile(const QVector<Slide> &slides, const QString &path, QString *errorMessage = nullptr);

// Reads a schedule previously written by saveToFile(). Returns true and
// fills `outSlides` on success; on failure `errorMessage` (if non-null)
// is set and `outSlides` is left unchanged.
bool loadFromFile(const QString &path, QVector<Slide> *outSlides, QString *errorMessage = nullptr);

} // namespace ScheduleIO
