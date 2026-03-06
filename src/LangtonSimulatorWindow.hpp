#pragma once

#include <QMainWindow>
#include <QTextEdit>
#include <QLineEdit>
#include <QAtomicInteger>
#include <QCheckBox>
#include <QElapsedTimer>
#include <QPushButton>

#include "FilePicker.hpp"
#include "RawGridView.hpp"
#include "PausableTimer.hpp"

#include "LangtonSimulationWorker.hpp"
#include "LangtonGenerateRandomPopup.hpp"

#include "langton/langton.hpp"

class LangtonSimulatorWindow : public QMainWindow
{
    Q_OBJECT

public:
    langton::state state = {};
    langton::examples_t examples = langton::make_langton_examples();

    PausableTimer simulationTimer;
    QAtomicInt simulationRunning = false;
    bool simulationStoppedVoluntarily = true; // Prevent first load from starting simulation until user has started a simulation for the first time
    u64 iterationsPerSecond = 1000;
    std::string stateInitialJson; // For reset functionality
    QString log;

    LangtonSimulationWorker *worker = nullptr;
    QThread *workerThread = nullptr;

    FilePicker *jsonPicker;
    QPushButton *btnGenerateRandExample;
    LangtonGenerateRandomPopup *generateRandomPopup;
    QComboBox *randExampleCombo;

    QCheckBox *normalizeCheckbox;

    u64 renderCount = 0;
    u64 lastRenderCount = 0;
    QTimer *renderPerSecTimer;
    QLineEdit *rendersPerSecEdit;

    QComboBox *simulationTypeCombo;

    // Tps = (simulation) ticks per second

    u64 lastIterationCount = 0;
    QTimer *actualTpsTimer;
    u64 actualTpsTimerIntervalMsec = 500;
    QLineEdit *actualTpsEdit;

    QPushButton *btnBackTwice;
    QPushButton *btnBackOnce;
    QPushButton *btnStop;
    QPushButton *btnForwardOnce;
    QPushButton *btnForwardTwice;

    QLineEdit *gridDimEdit;
    QLineEdit *antPosEdit;
    QLineEdit *orientationEdit;
    QLineEdit *lastStepResEdit;
    QLineEdit *maxvalEdit;
    QLineEdit *iterationEdit;

    QTextEdit *rulesEdit;
    QTextEdit *logEdit;

    RawGridView *viewer;

    explicit LangtonSimulatorWindow(QWidget *parent = nullptr, QString const &title = "Langton's Ant Simulator");

    ~LangtonSimulatorWindow();

    void stopWorker();

    bool importJsonFile(QString const &path);

    std::vector<std::string> importJsonString(std::string const &contents);

    void addLog(QString const &entry);

    bool loadExample(char const *example_name, std::string const &example_json);

    bool loadRandomExample();

    void startSimulation();

    void resumeSimulation();

    bool stopSimulation(bool voluntary = false);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
};
