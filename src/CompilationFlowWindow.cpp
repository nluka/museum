#include <QSplitter>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QTreeWidget>
#include <QWheelEvent>
#include <QPointF>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QGraphicsSceneMouseEvent>
#include <QDebug>

#include "CompilationFlowWindow.hpp"

class AstScene : public QGraphicsScene
{
public:
    AstScene(QObject *parent = nullptr)
        : QGraphicsScene(parent)
    {
    }

    void centerViewOnItems(QGraphicsView *view)
    {
        if (!view)
            return;

        QGraphicsScene *scene = view->scene();
        if (!scene)
            return;

        QRectF itemsRect = scene->itemsBoundingRect();
        if (itemsRect.isNull()) {
            view->centerOn(0, 0);
            return;
        }
        qreal pad = 20.0;
        itemsRect.adjust(-pad, -pad, pad, pad);

        view->setSceneRect(itemsRect);
        view->centerOn(itemsRect.center());
    }

    void addNode(QGraphicsItem *node)
    {
        addItem(node);
        // Expand scene rect if necessary
        QRectF rect = sceneRect().united(node->sceneBoundingRect().adjusted(-50, -50, 50, 50));
        setSceneRect(rect);
    }

    void addOriginCrosshair(qreal offset_x = 0, qreal offset_y = 0, qreal radius = 10)
    {
        addLine(offset_x, offset_y, 0 + offset_x, radius + offset_y);
        addLine(offset_x, offset_y, radius + offset_x, 0 + offset_y);
        addLine(offset_x, offset_y, 0 + offset_x, -radius + offset_y);
        addLine(offset_x, offset_y, -radius + offset_x, 0 + offset_y);
    }

protected:
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override
    {
        QGraphicsScene::mouseMoveEvent(event);

        QPointF p = event->scenePos();
        QRectF rect = sceneRect();

        // Expand dynamically when near edges
        qreal pad = 50.0;
        if (!rect.contains(p)) {
            rect.adjust(-pad, -pad, pad, pad);
            setSceneRect(rect);
        }
    }
};

class ZoomableGraphicsView : public QGraphicsView
{
public:
    ZoomableGraphicsView(QGraphicsScene *scene, QWidget *parent = nullptr)
        : QGraphicsView(scene, parent)
    {
        setRenderHint(QPainter::Antialiasing);
        setDragMode(QGraphicsView::ScrollHandDrag);
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    }

protected:
    void wheelEvent(QWheelEvent *event) override
    {
        double zoomInFactor = 1.15;
        double zoomOutFactor = 1.0 / zoomInFactor;

        // Zoom direction
        double factor = (event->angleDelta().y() > 0) ? zoomInFactor : zoomOutFactor;

        // Prevent too much zooming out
        double currentScale = transform().m11(); // uniform scale
        if (currentScale < 0.1 && factor < 1.0)
            return;
        if (currentScale > 10.0 && factor > 1.0)
            return;

        scale(factor, factor);
    }
};

class AstNodeRect : public QObject, public QGraphicsRectItem
{
    Q_OBJECT

public:
    AstScene *parentScene = nullptr;
    QString nodeName;

    AstNodeRect(QString const &nodeName_, QRectF const &rect, AstScene *parentScene_)
        : QGraphicsRectItem(rect), nodeName(nodeName_), parentScene(parentScene_)
    {
        setBrush(QBrush(Qt::white));
        setPen(QPen(Qt::black, 2));
        setFlag(QGraphicsItem::ItemIsSelectable, true);
        setAcceptHoverEvents(true);
    }

signals:
    void clicked(AstNodeRect *self);  // Emit pointer to self when clicked

protected:
    QVariant itemChange(GraphicsItemChange change, QVariant const &value) override
    {
        if (change == QGraphicsItem::ItemSelectedHasChanged) {
            if (this->isSelected()) {
                setBrush(Qt::yellow); // selected color
            }
            else {
                setBrush(Qt::white); // deselected color
            }
        }
        return QGraphicsRectItem::itemChange(change, value);
    }

    void mousePressEvent(QGraphicsSceneMouseEvent *event) override
    {
        for (QGraphicsItem *item : parentScene->selectedItems()) {
            if (item != this) {
                item->setSelected(false);
            }
        }
        setSelected(!isSelected());
        event->accept(); // IMPORTANT: tells Qt we handled it
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override
    {
        event->accept(); // IMPORTANT: prevents Qt from re-selecting the item
    }

    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override
    {
        setCursor(Qt::PointingHandCursor);
        QGraphicsRectItem::hoverEnterEvent(event);
    }

    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override
    {
        unsetCursor();
        QGraphicsRectItem::hoverLeaveEvent(event);
    }
};

AstNodeRect *createNode(AstScene *scene, QString const &name, qreal x, qreal y, CompilationFlowWindow *window)
{
    AstNodeRect *rect = new AstNodeRect(name, QRectF(0, 0, 120, 50), scene);
    rect->setPos(x, y);
    scene->addItem(rect);

    QGraphicsTextItem *label = new QGraphicsTextItem(name, rect);

    // Center text
    QRectF r = rect->rect();
    QRectF t = label->boundingRect();
    label->setPos((r.width() - t.width()) / 2,
                  (r.height() - t.height()) / 2);

    window->connect(rect, &AstNodeRect::clicked, window, [](AstNodeRect *rect){
        qDebug() << "Node clicked:" << rect->nodeName;
    });

    return rect;
}

QPointF bottomCenter(QGraphicsRectItem *item) {
    QRectF r = item->rect();
    return item->mapToScene(r.center().x(), r.bottom());
}

QPointF topCenter(QGraphicsRectItem *item) {
    QRectF r = item->rect();
    return item->mapToScene(r.center().x(), r.top());
}

void connectNodes(QGraphicsScene *scene, QGraphicsRectItem *parent, QGraphicsRectItem *child)
{
    QPointF p1 = bottomCenter(parent);
    QPointF p2 = topCenter(child);

    QGraphicsLineItem *line = scene->addLine(QLineF(p1, p2), QPen(Qt::black, 2));
    Q_UNUSED(line);
}

CompilationFlowWindow::CompilationFlowWindow(QWidget *parent, QString const &title)
    : QWidget(nullptr)
{
    Q_UNUSED(parent);

    setWindowTitle(title);

    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);

    AstScene *scene = new AstScene(this);
    {
        // Example binary tree layout
        // Level spacing
        const int levelY = 100;
        const int hSpacing = 150;

        // Root
        AstNodeRect *root = createNode(scene, "Root", 400, 0, this);

        // Level 1
        AstNodeRect *left1 = createNode(scene, "Left1", root->x() - hSpacing, root->y() + levelY, this);
        AstNodeRect *right1 = createNode(scene, "Right1", root->x() + hSpacing, root->y() + levelY, this);

        connectNodes(scene, root, left1);
        connectNodes(scene, root, right1);

        // Level 2
        AstNodeRect *left2 = createNode(scene, "Left2", left1->x() - hSpacing/2, left1->y() + levelY, this);
        AstNodeRect *right2 = createNode(scene, "Right2", left1->x() + hSpacing/2, left1->y() + levelY, this);

        AstNodeRect *left3 = createNode(scene, "Left3", right1->x() - hSpacing/2, right1->y() + levelY, this);
        AstNodeRect *right3 = createNode(scene, "Right3", right1->x() + hSpacing/2, right1->y() + levelY, this);

        connectNodes(scene, left1, left2);
        connectNodes(scene, left1, right2);
        connectNodes(scene, right1, left3);
        connectNodes(scene, right1, right3);
    }
    ZoomableGraphicsView *view = new ZoomableGraphicsView(scene);
    view->setWindowTitle("AST Tree Example");
    scene->centerViewOnItems(view);

    // Create 4 widgets to act as panes
    QWidget *pane1 = new QTextEdit("Pane 1");
    QWidget *pane2 = new QTextEdit("Pane 2");
    // QWidget *pane3 = new QTextEdit("Pane 3");
    QWidget *pane4 = new QTextEdit("Pane 4");

    // Add panes to splitter
    splitter->addWidget(pane1);
    splitter->addWidget(pane2);
    splitter->addWidget(view);
    splitter->addWidget(pane4);

    // Set splitter as central widget
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(splitter);
}

#include <CompilationFlowWindow.moc>
