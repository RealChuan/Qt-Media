#ifndef FFMEPG_GLOBAL_H
#define FFMEPG_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(FFMPEG_LIBRARY)
#  define FFMPEG_EXPORT Q_DECL_EXPORT
#elif  defined(FFMPEG_STATIC_LIBRARY) // Abuse single files for manual tests
#  define FFMPEG_EXPORT
#else
#  define FFMPEG_EXPORT Q_DECL_IMPORT
#endif

#endif // FFMEPG_GLOBAL_H
