#include <algorithm>

#include <QDebug>
#include <QThread>
#include <QElapsedTimer>

#include "LangtonSimulationWorker.hpp"

LangtonSimulationWorker::LangtonSimulationWorker(langton::state *s)
    : state(s), running(false)
{}

void LangtonSimulationWorker::run_forward_adaptive(u64 startingIterationCount, u64 startingIterationsPerSecond, u64 updateFrequencyHz)
{
    using clock = std::chrono::steady_clock;

    auto lastStepTime = clock::now();
    auto lastUpdateTime = clock::now();

    iterationCount = startingIterationCount;
    iterationsPerSecond = startingIterationsPerSecond;

    double estimatedThroughput = double(iterationsPerSecond);
    const double alpha = 0.1;

    assert(updateFrequencyHz > 0);
    const double updatePeriodSec = 1.0 / double(updateFrequencyHz);

    while (running) {
        auto now = clock::now();
        auto nextWakeTime = now + std::chrono::microseconds(500);

        double elapsedSec = std::chrono::duration<double>(now - lastStepTime).count();

        double dynamicMaxBatch = std::clamp(estimatedThroughput * 0.1, 1000.0, 1000000.0);

        u64 stepsToDo = std::min<u64>(u64(elapsedSec * iterationsPerSecond), u64(dynamicMaxBatch));

        if (stepsToDo > 0) {
            for (u64 i = 0; i < stepsToDo; ++i) {
                state->last_step_res = langton::attempt_step_forward(*state);
                ++iterationCount;
                ++state->generation;
                if (state->last_step_res == langton::step_result::HIT_EDGE) {
                    running = false;
                    break;
                }
            }

            double batchTime = std::chrono::duration<double>(clock::now() - lastStepTime).count();
            if (batchTime > 1e-9) {
                double batchThroughput = double(stepsToDo) / batchTime;
                estimatedThroughput = alpha * batchThroughput + (1.0 - alpha) * estimatedThroughput;
            }

            lastStepTime = now;
        }

        double sinceUpdate = std::chrono::duration<double>(now - lastUpdateTime).count();
        if (sinceUpdate >= updatePeriodSec && stepsToDo > 0) {
            emit updated(state->ant_col, state->ant_row, state->ant_orientation, state->last_step_res, iterationCount);
            lastUpdateTime = now;
        }

        std::this_thread::sleep_until(nextWakeTime);
        nextWakeTime += std::chrono::microseconds(500);
    }

    emit updated(state->ant_col, state->ant_row, state->ant_orientation, state->last_step_res, iterationCount);
    emit finished();
}

void LangtonSimulationWorker::run_forward_unrestricted(u64 startingIterationCount, u64 startingIterationsPerSecond)
{
    using clock = std::chrono::steady_clock;
    auto lastTime = clock::now();

    iterationCount = startingIterationCount;
    iterationsPerSecond = startingIterationsPerSecond;

    while (running) {
        auto now = clock::now();
        double elapsedSec = std::chrono::duration<double>(now - lastTime).count();
        u64 stepsToDo = u64(elapsedSec * iterationsPerSecond);
        if (stepsToDo > 0) {
            for (u64 i = 0; i < stepsToDo; ++i) {
                state->last_step_res = langton::attempt_step_forward(*state);
                ++iterationCount;
                ++state->generation;
                if (state->last_step_res == langton::step_result::HIT_EDGE) {
                    running = false;
                    break;
                }
            }
            emit updated(state->ant_col, state->ant_row, state->ant_orientation, state->last_step_res, iterationCount);
            lastTime = now;
        }
        std::this_thread::sleep_until(clock::now() + std::chrono::microseconds(500));
    }

    emit finished();
}
