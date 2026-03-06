#pragma once

#include <QElapsedTimer>

class PausableTimer
{
public:
    void start() {
        elapsedBeforePause = 0;
        timer.start();
        paused = false;
    }

    qint64 restart() {
        qint64 e = elapsed();
        elapsedBeforePause = 0;
        timer.restart();
        paused = false;
        return e;
    }

    void pause() {
        if (!paused) {
            elapsedBeforePause += timer.elapsed();
            paused = true;
        }
    }

    void resume() {
        if (paused) {
            timer.restart();
            paused = false;
        }
    }

    qint64 elapsed() const {
        if (paused)
            return elapsedBeforePause;
        return elapsedBeforePause + timer.elapsed();
    }

private:
    QElapsedTimer timer;
    qint64 elapsedBeforePause = 0;
    bool paused = false;
};
