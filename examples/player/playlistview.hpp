#ifndef PLAYLISTVIEW_HPP
#define PLAYLISTVIEW_HPP

#include <QListView>

class PlayListView : public QListView
{
public:
    explicit PlayListView(QWidget *parent = nullptr);
    ~PlayListView();

    QModelIndexList selectedAllIndexs() const;
};

#endif // PLAYLISTVIEW_HPP
