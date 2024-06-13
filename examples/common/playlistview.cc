#include "playlistview.hpp"

#include <QFileInfo>
#include <QUrl>

auto isPlaylist(const QUrl &url) -> bool // Check for ".m3u" playlists.
{
    if (!url.isLocalFile()) {
        return false;
    }
    const QFileInfo fileInfo(url.toLocalFile());
    return fileInfo.exists()
           && (fileInfo.suffix().compare(QLatin1String("m3u"), Qt::CaseInsensitive) == 0);
}

PlayListView::PlayListView(QWidget *parent)
    : QListView(parent)
{
    setAlternatingRowColors(true);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
}

PlayListView::~PlayListView() {}

QModelIndexList PlayListView::selectedAllIndexs() const
{
    return selectedIndexes();
}
