#include <QFontDatabase>
#include <QHeaderView>

#include "HexView.hpp"

QTableView *createHexView(const u8 *data, u64 size, QWidget *parent)
{
    QTableView *view = new QTableView(parent);

    view->setModel(new HexTableModel(data, size, view));
    view->setItemDelegate(new HexDelegate(view));

    view->setShowGrid(false);
    view->verticalHeader()->setVisible(false);
    view->horizontalHeader()->setVisible(false);
    view->setSelectionMode(QAbstractItemView::NoSelection);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    view->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    return view;
}

HexTableModel::HexTableModel(const u8 *data, u64 size, QObject *parent)
    : QAbstractTableModel(parent)
    , m_data(data)
    , m_size(size)
{}

i32 HexTableModel::rowCount(const QModelIndex &) const
{
    return (int)((m_size + 15) / 16);
}

i32 HexTableModel::columnCount(const QModelIndex &) const
{
    return 1;
}

QVariant HexTableModel::data(const QModelIndex &, i32 role) const
{
    if (role == Qt::DisplayRole)
        return {};

    return {};
}

HexDelegate::HexDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
    initTables();
}

void HexDelegate::paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &index) const
{
    const HexTableModel *model = static_cast<const HexTableModel *>(index.model());

    const u8 *data = model->m_data;
    u64 size = model->m_size;

    const u64 row = index.row();
    const u64 base = row * 16;

    if (base >= size)
        return;

    const u8 *rowPtr = data + base;

    p->save();

    if (opt.state & QStyle::State_Selected)
        p->fillRect(opt.rect, opt.palette.highlight());
    else
        p->fillRect(opt.rect, opt.palette.base());

    const QFontMetrics &fm = opt.fontMetrics;

    const i32 charWidth   = fm.horizontalAdvance('0');
    const i32 hexWidth    = fm.horizontalAdvance("FF ");
    const i32 offsetWidth = fm.horizontalAdvance("00000000 ");

    const i32 paddingX = 10;
    const i32 paddingY = 10;

    i32 x = opt.rect.x() + paddingX;
    i32 y = opt.rect.y() + fm.ascent() + paddingY;

    // --- OFFSET ---
    drawHex32(p, x, y, static_cast<u32>(base));
    x += offsetWidth;

    // --- HEX ---
    for (i32 i = 0; i < 16; ++i) {
        if (base + i >= size)
            break;

        drawHex8(p, x, y, rowPtr[i]);
        x += hexWidth;
    }

    x += charWidth;

    // --- ASCII ---
    for (i32 i = 0; i < 16; ++i) {
        if (base + i >= size)
            break;

        const QChar c = m_ascii[rowPtr[i]];
        QChar buf[1] = { c };
        p->drawText(x, y, QString(buf, 1));
        x += charWidth;
    }

    p->restore();
}

QSize HexDelegate::sizeHint(const QStyleOptionViewItem &opt,
                           const QModelIndex &) const
{
    const QFontMetrics &fm = opt.fontMetrics;
    return QSize(0, fm.height());
}

void HexDelegate::initTables()
{
    const char *hex = "0123456789ABCDEF";

    for (i32 i = 0; i < 256; ++i) {
        m_hex[i][0] = hex[(i >> 4) & 0xF];
        m_hex[i][1] = hex[i & 0xF];

        m_ascii[i] = (i >= 32 && i <= 126) ? QChar(i) : '.';
    }
}

void HexDelegate::drawHex8(QPainter *p, i32 x, i32 y, u8 v) const
{
    char buf[2];
    buf[0] = m_hex[v][0];
    buf[1] = m_hex[v][1];

    p->drawText(x, y, QString::fromLatin1(buf, 2));
}

void HexDelegate::drawHex32(QPainter *p, i32 x, i32 y, u32 v) const
{
    char buf[8];

    for (i32 i = 0; i < 8; ++i) {
        i32 shift = (7 - i) * 4;
        buf[i] = "0123456789ABCDEF"[(v >> shift) & 0xF];
    }

    p->drawText(x, y, QString::fromLatin1(buf, 8));
}
