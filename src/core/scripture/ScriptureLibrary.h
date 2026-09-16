#pragma once

#include <QString>
#include <QVector>

// One verse in a loaded translation, flattened out of KJV.json's nested
// book -> chapter -> verse structure into a single, directly-indexable
// list -- the natural shape for a scrolling reference table (see
// ui/ScripturePanel) and for linear canonical-order search/lookup.
struct ScriptureVerse
{
    QString book;
    int chapter = 0;
    int verse = 0;
    QString text;
    QString reference; // precomputed "Book Chapter:Verse", e.g.
                        // "Genesis 1:1" -- so the UI never has to
                        // reformat this on every row of a 31,000+ row
                        // table.
};

// ScriptureLibrary loads and caches the one bundled translation (KJV
// today -- see ui/ScripturePanel's doc comment for why only one ships
// right now) from the Qt resource embedded at build time (see
// resources/Scriptures.qrc), so it's always available regardless of
// the app's working directory or install layout -- unlike Media tab
// imports (core/storage/MediaLibraryStore), which deliberately
// reference external files the operator picked, this is bundled
// content the app ships with.
//
// Kept in core/ (no Qt widgets) so it stays reusable from a future
// headless mode, same rationale as ScheduleIO/MediaLibraryStore.
namespace ScriptureLibrary
{

// Short display code for the one bundled translation, e.g. "KJV".
QString translationCode();

// Loads (once; cached thereafter) and returns every verse in the
// bundled translation, in canonical Bible order (Genesis 1:1 first,
// Revelation last). Returns an empty list (logged, not fatal) if the
// embedded resource is somehow missing or malformed -- the Scriptures
// tab should degrade to "no verses found", never crash the app.
const QVector<ScriptureVerse> &verses();

} // namespace ScriptureLibrary
