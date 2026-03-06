#pragma once

#include <QWidget>

class QSpinBox;
class QLineEdit;
class QComboBox;
class QKeyEvent;

class LangtonGenerateRandomPopup : public QWidget
{
    Q_OBJECT

public:
    explicit LangtonGenerateRandomPopup(QWidget *parent = nullptr);

    void openAt(const QPoint &globalPos);

    int gridWidth() const;
    int gridHeight() const;
    QString turnsText() const;
    QString orientationsText() const;
    QString shadeOrderValue() const;

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    QSpinBox *gridWidthSpin;
    QSpinBox *gridHeightSpin;

    QLineEdit *turnsInput;
    QLineEdit *orientationInput;

    QComboBox *shadeOrderCombo;
};
