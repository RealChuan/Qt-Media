#ifndef MPV_GLOBAL_H
#define MPV_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(MPV_LIBRARY)
#define MPV_LIB_EXPORT Q_DECL_EXPORT
#else
#define MPV_LIB_EXPORT Q_DECL_IMPORT
#endif

#endif // MPV_GLOBAL_H
