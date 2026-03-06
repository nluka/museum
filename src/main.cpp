#define _CRT_SECURE_NO_WARNINGS // for getenv

#include <cstdlib>

#include <QApplication>
#include <QDebug>
#include <QDateTime>

#include "util.hpp"

#include "StartWindow.hpp"

extern "C" void __asan_init();

int main(int argc, char *argv[])
{
    auto asan_link_check = []() {
        __asan_init(); // will link only if ASan is present
        assert(false && "Function should never be called.");
    };
    Q_UNUSED(asan_link_check);

    qDebug() << "ASAN_OPTIONS =" << getenv("ASAN_OPTIONS");

    seed_fast_rand(u64(QDateTime::currentMSecsSinceEpoch()));

    QApplication a(argc, argv);
    StartWindow w;
    w.show();
    return a.exec();
}
