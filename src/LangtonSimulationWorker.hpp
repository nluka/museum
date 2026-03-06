#pragma once

#include <chrono>

#include <QObject>
#include <QAtomicInteger>

#include "langton/langton.hpp"

class LangtonSimulationWorker : public QObject {
    Q_OBJECT
public:
    explicit LangtonSimulationWorker(langton::state *s);

    langton::state *state;
    QAtomicInteger<bool> running;
    QAtomicInteger<quint64> iterationsPerSecond;
    QAtomicInteger<quint64> iterationCount;

signals:
    void updated(u64 antCol, u64 antRow,
                 langton::orientation::value_type ori,
                 langton::step_result::value_type lastStep,
                 u64 iterCount);

    void finished();

public slots:
    void run_forward_adaptive(u64 startingIterationCount, u64 startingIterationsPerSecond, u64 updateFrequencyHz);
    void run_forward_unrestricted(u64 startingIterationCount, u64 startingIterationsPerSecond);
};
