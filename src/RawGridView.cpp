#include "RawGridView.hpp"

RawGridView::RawGridView(u8 *buffer, int w, int h, QWidget *parent)
    : QWidget(parent),
    img(buffer, w, h, w * sizeof(u8), QImage::Format_Grayscale8)
{
    setMouseTracking(true);
    setMinimumSize(100, 100);
}

void RawGridView::setBuffer(u8 *buffer, int w, int h, u8 maxval, bool normalize)
{
    if (!normalize || maxval == 255) {
        img = QImage(buffer, w, h, w * sizeof(u8), QImage::Format_Grayscale8);
    }
    else { // normalize to 0-255 range for better contrast - display-time intensity stretch (contrast normalization)
        img = QImage(w, h, QImage::Format_Grayscale8);

        for (int y = 0; y < h; ++y) {
            const u8 *src = buffer + y * w;
            u8 *dst = img.scanLine(y);

            for (int x = 0; x < w; ++x) {
                dst[x] = static_cast<u8>((src[x] * 255) / maxval);
            }
        }
    }
}

void RawGridView::refresh()
{
    update();
}

void RawGridView::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::SmoothPixmapTransform, false);

    p.translate(panX, panY);   // screen-space pan
    p.scale(scaleFactor, scaleFactor);

    p.drawImage(0, 0, img);
}

void RawGridView::wheelEvent(QWheelEvent *event)
{
    const float zoomSpeed = 1.15f;
    float oldScale = scaleFactor;

    // Determine zoom factor
    if (event->angleDelta().y() > 0)
        scaleFactor *= zoomSpeed;
    else
        scaleFactor /= zoomSpeed;

    if (scaleFactor < 0.05f) scaleFactor = 0.05f;
    if (scaleFactor > 80.0f) scaleFactor = 80.0f;

    // Get cursor position relative to widget
    QPointF mousePos = event->pos();

    // Compute offset so the point under cursor stays fixed
    // worldPos = (screenPos - pan) / oldScale
    QPointF worldPos = (mousePos - QPointF(panX, panY)) / oldScale;

    // new pan = screenPos - worldPos * newScale
    panX = mousePos.x() - worldPos.x() * scaleFactor;
    panY = mousePos.y() - worldPos.y() * scaleFactor;

    update();
}

void RawGridView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        panning = true;
        lastMouse = event->pos();
    }
}

void RawGridView::mouseMoveEvent(QMouseEvent *event)
{
    if (panning) {
        QPointF delta = event->pos() - lastMouse;
        lastMouse = event->pos();
        panX += delta.x();
        panY += delta.y();
        update();
    }
}

void RawGridView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        panning = false;
}

void RawGridView::center(int margin)
{
    if (img.isNull())
        return;

    const float widgetW = width();
    const float widgetH = height();

    const float imgW = img.width();
    const float imgH = img.height();

    if (imgW <= 0 || imgH <= 0)
        return;

    const float availW = widgetW - 2.0f * margin;
    const float availH = widgetH - 2.0f * margin;

    // Always fit to available area
    const float sx = availW / imgW;
    const float sy = availH / imgH;

    scaleFactor = std::min(sx, sy);

    const float scaledW = imgW * scaleFactor;
    const float scaledH = imgH * scaleFactor;

    // Center in screen-space
    panX = (widgetW - scaledW) * 0.5f;
    panY = (widgetH - scaledH) * 0.5f;

    update();
}
