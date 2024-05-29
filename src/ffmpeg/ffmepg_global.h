#ifndef FFMEPG_GLOBAL_H
#define FFMEPG_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(FFMPEG_LIBRARY)
#  define FFMPEG_EXPORT Q_DECL_EXPORT
#else
#  define FFMPEG_EXPORT Q_DECL_IMPORT
#endif

#endif // FFMEPG_GLOBAL_H
