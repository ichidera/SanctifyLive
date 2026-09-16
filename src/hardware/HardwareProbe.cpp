#include "hardware/HardwareProbe.h"

#include <QFile>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QRegularExpression>
#include <QThread>

#if defined(Q_OS_WIN)
#include <windows.h>
#elif defined(Q_OS_LINUX)
// /proc/meminfo is parsed directly (see detectTotalRamMb() below) --
// no extra header needed beyond QFile.
#endif

// GL_NVX_gpu_memory_info's token, spelled out by hand: this extension
// has no Qt wrapper and pulling in the full OpenGL extension-loader
// headers just for one enum isn't worth it. The value itself is part of
// the (stable, published) NVX_gpu_memory_info spec, not something that
// changes per-driver.
#ifndef GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX
#define GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX 0x9047
#endif

namespace HardwareProbe
{

namespace
{

#if defined(Q_OS_LINUX)
qint64 detectTotalRamMb()
{
    QFile file(QStringLiteral("/proc/meminfo"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return 0;

    static const QRegularExpression pattern(QStringLiteral(R"(MemTotal:\s*(\d+)\s*kB)"));
    const QRegularExpressionMatch match = pattern.match(QString::fromLatin1(file.readAll()));
    if (!match.hasMatch())
        return 0;
    return match.captured(1).toLongLong() / 1024;
}
#elif defined(Q_OS_WIN)
qint64 detectTotalRamMb()
{
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    if (!GlobalMemoryStatusEx(&status))
        return 0;
    return static_cast<qint64>(status.ullTotalPhys / (1024 * 1024));
}
#else
qint64 detectTotalRamMb()
{
    return 0; // unknown on this platform; TierScorer treats 0 conservatively, not as a crash
}
#endif

void detectOpenGl(HardwareProfile *profile)
{
    QOpenGLContext context;
    if (!context.create())
        return; // no usable OpenGL at all -- profile stays openglAvailable = false

    QOffscreenSurface surface;
    surface.setFormat(context.format());
    surface.create();
    if (!surface.isValid() || !context.makeCurrent(&surface))
        return;

    profile->openglAvailable = true;

    QOpenGLFunctions *gl = context.functions();
    const auto *rendererStr = reinterpret_cast<const char *>(gl->glGetString(GL_RENDERER));
    const auto *versionStr = reinterpret_cast<const char *>(gl->glGetString(GL_VERSION));
    profile->openglRenderer = rendererStr ? QString::fromLatin1(rendererStr) : QString();
    profile->openglVersion = versionStr ? QString::fromLatin1(versionStr) : QString();

    // VRAM has no vendor-neutral core-OpenGL query. GL_NVX_gpu_memory_info
    // is NVIDIA-proprietary-driver-only -- its absence (AMD, Intel, Mesa
    // software rasterizers, most laptops) is the common case, not a
    // detection failure, which is why vramMb defaults to -1 ("unknown")
    // rather than 0 ("no VRAM").
    if (context.hasExtension("GL_NVX_gpu_memory_info")) {
        GLint dedicatedKb = 0;
        gl->glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &dedicatedKb);
        if (dedicatedKb > 0)
            profile->vramMb = dedicatedKb / 1024;
    }

    context.doneCurrent();
}

} // namespace

HardwareProfile detect()
{
    HardwareProfile profile;
    profile.cpuCores = qMax(1, QThread::idealThreadCount());
    profile.totalRamMb = detectTotalRamMb();
    detectOpenGl(&profile);
    return profile;
}

} // namespace HardwareProbe
