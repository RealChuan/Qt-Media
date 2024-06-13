#ifndef PLAYLISTVIEW_HPP
#define PLAYLISTVIEW_HPP

#include <QListView>

auto isPlaylist(const QUrl &url) -> bool;

class PlayListView : public QListView
{
public:
    explicit PlayListView(QWidget *parent = nullptr);
    ~PlayListView() override;

    [[nodiscard]] auto selectedAllIndexs() const -> QModelIndexList;
};

#endif // PLAYLISTVIEW_HPP
