#pragma once

#include <QWidget>
#include <QImage>
#include <QPoint>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QPainter>

typedef unsigned char u8;

class RawGridView : public QWidget
{
    Q_OBJECT

public:
    RawGridView(u8 *buffer, int w, int h, QWidget *parent = nullptr);

    void setBuffer(u8 *buffer, int w, int h, u8 maxval, bool normalize);
    void refresh();
    void center(int margin_px);

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QImage img;

    float scaleFactor = 1.0f;
    float panX = 0.0f;
    float panY = 0.0f;

    bool panning = false;
    QPoint lastMouse;
};
