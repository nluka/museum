#include "LangtonGenerateRandomPopup.hpp"

#include <QSpinBox>
#include <QLineEdit>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QRegularExpressionValidator>
#include <QRegularExpression>
#include <QKeyEvent>
#include <QApplication>
#include <QScreen>
#include <QGraphicsDropShadowEffect>

LangtonGenerateRandomPopup::LangtonGenerateRandomPopup(QWidget *parent)
    : QWidget(parent, Qt::Popup)
{
    // setWindowFlag(Qt::FramelessWindowHint);
    setFocusPolicy(Qt::StrongFocus);

    auto *layout = new QGridLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setHorizontalSpacing(6);
    layout->setVerticalSpacing(4);
    layout->setSizeConstraint(QLayout::SetFixedSize);

    gridWidthSpin = new QSpinBox(this);
    gridWidthSpin->setRange(2, 10000);
    gridWidthSpin->setValue(2000);
    gridWidthSpin->setAccelerated(true);

    gridHeightSpin = new QSpinBox(this);
    gridHeightSpin->setRange(2, 10000);
    gridHeightSpin->setValue(2000);
    gridHeightSpin->setAccelerated(true);

    turnsInput = new QLineEdit(this);
    turnsInput->setPlaceholderText("RL");
    turnsInput->setText("RL");
    turnsInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[RL]*"), turnsInput));

    orientationInput = new QLineEdit(this);
    orientationInput->setPlaceholderText("NESW");
    orientationInput->setText("NESW");
    orientationInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[NESW]*"), orientationInput));

    shadeOrderCombo = new QComboBox(this);
    shadeOrderCombo->addItems({"rand", "asc", "desc"});

    layout->addWidget(new QLabel("Grid Width"), 0, 0);
    layout->addWidget(gridWidthSpin, 0, 1);

    layout->addWidget(new QLabel("Grid Height"), 1, 0);
    layout->addWidget(gridHeightSpin, 1, 1);

    layout->addWidget(new QLabel("Turns"), 2, 0);
    layout->addWidget(turnsInput, 2, 1);

    layout->addWidget(new QLabel("Direction"), 3, 0);
    layout->addWidget(orientationInput, 3, 1);

    layout->addWidget(new QLabel("Rules Order"), 4, 0);
    layout->addWidget(shadeOrderCombo, 4, 1);
}

void LangtonGenerateRandomPopup::openAt(const QPoint &globalPos)
{
    adjustSize();

    QScreen *screen = QGuiApplication::screenAt(globalPos);
    if (!screen)
        screen = QGuiApplication::primaryScreen();

    QRect screenRect = screen->availableGeometry();

    QPoint pos = globalPos;

    if (pos.x() + width() > screenRect.right())
        pos.setX(screenRect.right() - width());

    if (pos.y() + height() > screenRect.bottom())
        pos.setY(screenRect.bottom() - height());

    move(pos);

    show();

    gridWidthSpin->setFocus();
    gridWidthSpin->selectAll();
}

void LangtonGenerateRandomPopup::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
    {
        close();
        return;
    }

    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        close();
        return;
    }

    QWidget::keyPressEvent(event);
}

int LangtonGenerateRandomPopup::gridWidth() const
{
    return gridWidthSpin->value();
}

int LangtonGenerateRandomPopup::gridHeight() const
{
    return gridHeightSpin->value();
}

QString LangtonGenerateRandomPopup::turnsText() const
{
    return turnsInput->text();
}

QString LangtonGenerateRandomPopup::orientationsText() const
{
    return orientationInput->text();
}

QString LangtonGenerateRandomPopup::shadeOrderValue() const
{
    return shadeOrderCombo->currentText();
}
