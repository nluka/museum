#include <QLocale>

#include "FactorSpinBox.hpp"

void FactorSpinBox::stepBy(int steps)
{
    double v = value();
    if (steps > 0)
        setValue(v * 10.0);
    else
        setValue(v / 10.0);
}

FactorSpinBox::FactorSpinBox(QWidget *parent, QString suffix_)
    : QDoubleSpinBox(parent),
      mySuffix(suffix_)
{
    setLocale(QLocale::system());
}

QString FactorSpinBox::textFromValue(double value) const
{
    QLocale loc = locale();
    qlonglong v = static_cast<qlonglong>(value);

    struct Scale { qlonglong divisor; const char *name; };

    static const Scale scales[] = {
        {1'000'000'000'000LL, "trillion"},
        {1'000'000'000LL,     "billion"},
        {1'000'000LL,         "million"},
        {1'000LL,             "thousand"}
    };

    for (Scale const &s : scales) {
        if (v >= s.divisor) {
            return loc.toString(v) + " (" + loc.toString(v / s.divisor) + " " + s.name + ") ticks/sec";
        }
    }

    return loc.toString(v) + " ticks/sec";
}

double FactorSpinBox::valueFromText(const QString &text) const
{
    QLocale loc = locale();

    QString number = text;

    int paren = number.indexOf('(');
    if (paren >= 0)
        number = number.left(paren);

    number.remove(mySuffix);
    number = number.trimmed();

    bool ok = false;
    qlonglong val = loc.toLongLong(number, &ok);

    if (!ok)
        return value();

    return val;
}

QValidator::State FactorSpinBox::validate(QString &text, int &pos) const
{
    Q_UNUSED(pos);

    QString number = text;

    int paren = number.indexOf('(');
    if (paren >= 0)
        number = number.left(paren);

    if (!mySuffix.isEmpty() && number.endsWith(mySuffix))
        number.chop(mySuffix.size());

    number = number.trimmed();

    bool ok;
    locale().toLongLong(number, &ok);

    if (ok)
        return QValidator::Acceptable;

    return QValidator::Intermediate;
}
