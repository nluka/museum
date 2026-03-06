#pragma once

#include <QDoubleSpinBox>

class FactorSpinBox : public QDoubleSpinBox {
    Q_OBJECT

public:
    using QDoubleSpinBox::QDoubleSpinBox;

    explicit FactorSpinBox(QWidget *parent = nullptr, QString suffix = "");

protected:
    QString mySuffix;

    void stepBy(int steps) override;
    QString textFromValue(double value) const override;
    double valueFromText(QString const &text) const override;
    QValidator::State validate(QString &text, int &pos) const override;
};
