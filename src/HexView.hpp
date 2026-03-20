#pragma once

#include <QAbstractTableModel>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QTableView>

#include "primitives.hpp"

QTableView *createHexView(const u8 *data, u64 size, QWidget *parent = nullptr);

class HexDelegate;

class HexTableModel : public QAbstractTableModel
{
public:
    HexTableModel(const u8 *data, u64 size, QObject *parent = nullptr);

    i32 rowCount(const QModelIndex &) const override;
    i32 columnCount(const QModelIndex &) const override;
    QVariant data(const QModelIndex &, i32 role) const override;

private:
    const u8 *m_data;
    u64 m_size;

    friend class HexDelegate;
};

class HexDelegate : public QStyledItemDelegate
{
public:
    HexDelegate(QObject *parent = nullptr);

    void paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &index) const override;

    QSize sizeHint(const QStyleOptionViewItem &opt, const QModelIndex &) const override;

private:
    char  m_hex[256][2];
    QChar m_ascii[256];

    void initTables();

    void drawHex8(QPainter *p, i32 x, i32 y, u8 v) const;
    void drawHex32(QPainter *p, i32 x, i32 y, u32 v) const;
};
