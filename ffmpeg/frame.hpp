#pragma once

struct AVFrame;

namespace Ffmpeg {

class Frame
{
public:
    explicit Frame();
    Frame(const Frame &other);
    Frame &operator=(const Frame &other);
    ~Frame();

    void clear();

    AVFrame *avFrame();

private:
    AVFrame *m_frame;
};

} // namespace Ffmpeg
