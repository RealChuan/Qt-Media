// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMEDIAPLAYLIST_H
#define QMEDIAPLAYLIST_H

#include <QtCore/qobject.h>

#include <QtMultimedia/qmediaenumdebug.h>
#include <QtMultimedia/qtmultimediaglobal.h>

QT_BEGIN_NAMESPACE

class QMediaPlaylistPrivate;
class QMediaPlaylist : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QMediaPlaylist::PlaybackMode playbackMode READ playbackMode WRITE setPlaybackMode
                   NOTIFY playbackModeChanged)
    Q_PROPERTY(QUrl currentMedia READ currentMedia NOTIFY currentMediaChanged)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)

public:
    enum PlaybackMode { CurrentItemOnce, CurrentItemInLoop, Sequential, Loop, Random };
    Q_ENUM(PlaybackMode)
    enum Error { NoError, FormatError, FormatNotSupportedError, NetworkError, AccessDeniedError };
    Q_ENUM(Error)

    explicit QMediaPlaylist(QObject *parent = nullptr);
    ~QMediaPlaylist() override;

    [[nodiscard]] auto playbackMode() const -> PlaybackMode;
    void setPlaybackMode(PlaybackMode mode);

    [[nodiscard]] auto currentIndex() const -> int;
    [[nodiscard]] auto currentMedia() const -> QUrl;

    [[nodiscard]] auto nextIndex(int steps = 1) const -> int;
    [[nodiscard]] auto previousIndex(int steps = 1) const -> int;

    [[nodiscard]] auto media(int index) const -> QUrl;

    [[nodiscard]] auto mediaCount() const -> int;
    [[nodiscard]] auto isEmpty() const -> bool;

    void addMedia(const QUrl &content);
    void addMedia(const QList<QUrl> &items);
    auto insertMedia(int index, const QUrl &content) -> bool;
    auto insertMedia(int index, const QList<QUrl> &items) -> bool;
    auto moveMedia(int from, int to) -> bool;
    auto removeMedia(int pos) -> bool;
    auto removeMedia(int start, int end) -> bool;
    void clear();

    void load(const QUrl &location, const char *format = nullptr);
    void load(QIODevice *device, const char *format = nullptr);

    auto save(const QUrl &location, const char *format = nullptr) const -> bool;
    auto save(QIODevice *device, const char *format) const -> bool;

    [[nodiscard]] auto error() const -> Error;
    [[nodiscard]] auto errorString() const -> QString;

public Q_SLOTS:
    void shuffle();

    void next();
    void previous();

    void setCurrentIndex(int index);

Q_SIGNALS:
    void currentIndexChanged(int index);
    void playbackModeChanged(QMediaPlaylist::PlaybackMode mode);
    void currentMediaChanged(const QUrl &);

    void mediaAboutToBeInserted(int start, int end);
    void mediaInserted(int start, int end);
    void mediaAboutToBeRemoved(int start, int end);
    void mediaRemoved(int start, int end);
    void mediaChanged(int start, int end);

    void loaded();
    void loadFailed();

private:
    QMediaPlaylistPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QMediaPlaylist)
};

QT_END_NAMESPACE

Q_MEDIA_ENUM_DEBUG(QMediaPlaylist, PlaybackMode)
Q_MEDIA_ENUM_DEBUG(QMediaPlaylist, Error)

#endif // QMEDIAPLAYLIST_H
