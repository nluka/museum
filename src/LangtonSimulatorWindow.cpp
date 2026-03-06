// TODO: center on ant, draw ant pos and orientation somehow
// TODO: save initial state to json file
// TODO: save current state to json file
// TODO: save current state to image file
// TODO: add turn around option when ant hits edge
// TODO: add wrap around option when ant hits edge
// TODO: add to gallery, export gallery

#include <QPushButton>
#include <QFormLayout>
#include <QComboBox>
#include <QSpinBox>
#include <QSplitter>
#include <QLineEdit>
#include <QScrollArea>
#include <QLabel>
#include <QDebug>
#include <QMessageBox>
#include <QTimer>
#include <QDateTime>
#include <QThread>
#include <QObject>
#include <QElapsedTimer>
#include <QMetaType>
#include <QMimeData>
#include <QFileInfo>

#include "FilePicker.hpp"
#include "FactorSpinBox.hpp"
#include "populateCommonMenuBar.hpp"
#include "util.hpp"

#include "LangtonSimulatorWindow.hpp"
#include "LangtonGenerateRandomPopup.hpp"

void showErrorDialogSimple(QWidget *parent, QString const &msg)
{
    QMessageBox box(parent);
    box.setIcon(QMessageBox::Critical);
    box.setWindowTitle("Error");
    box.setText(msg);
    box.setStandardButtons(QMessageBox::Ok);
    box.exec();
}

void showErrorDialogListErrors(QWidget *parent, QString const &msg, std::vector<std::string> const &errors)
{
    // Build the detailed text block
    QString details;
    for (u64 i = 0; i < errors.size(); ++i) {
        auto const &e = errors[i];
        details += QString("[%1] ").arg(i + 1) + QString::fromStdString(e) + "\n";
    }

    QMessageBox box(parent);
    box.setIcon(QMessageBox::Critical);
    box.setWindowTitle("Error");
    box.setText(msg);
    box.setDetailedText(details);
    box.setStandardButtons(QMessageBox::Ok);
    box.exec();
}

bool LangtonSimulatorWindow::loadRandomExample()
{
    u64 choice = fast_rand(0, examples.size() - 1);
    auto random_example = examples[choice];
    char const *random_example_name = std::get<0>(random_example);
    std::string random_example_json = std::get<1>(random_example);

    return loadExample(random_example_name, random_example_json);
}

QString rulesToText(langton::rules_t const &rules, u8 maxval)
{
    std::ostringstream os;

    auto maxval_digits = util::count_digits(maxval);

    auto const print_rule = [&os, maxval_digits](
        u64 const shade,
        langton::rule const &rule,
        b8 const comma_at_end)
    {
        os
        // << "    " // indentation
        // << "{ "
        << "on " << std::setw(i32(maxval_digits)) << std::setfill(' ') << shade << "  "
        << "replace_with " << std::setw(i32(maxval_digits)) << std::setfill(' ') << static_cast<u16>(rule.replacement_shade) << "  "
        << "turn " << langton::turn_direction::to_cstr(rule.turn_dir)
        // << " }"
        << (comma_at_end ? "\n" : "")
        // << '\n'
        ;
    };

    u64 last_rule_idx = 0;
    for (u64 i = 255; i > 0; --i) {
        if (rules[i].turn_dir != langton::turn_direction::NIL) {
            last_rule_idx = i;
            break;
        }
    }

    for (u64 shade = 0; shade < last_rule_idx; ++shade) {
        auto const &rule = rules[shade];
        b8 const exists = rule.turn_dir != langton::turn_direction::NIL;
        if (exists)
        print_rule(shade, rule, true);
    }
    print_rule(last_rule_idx, rules[last_rule_idx], false);

    QString rulesText;
    return rulesText.fromStdString(os.str());
}

bool LangtonSimulatorWindow::loadExample(char const *name, std::string const &json)
{
    bool wasRunning = stopSimulation();

    auto errors = importJsonString(json);

    QString currentDateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    if (errors.empty()) {
        addLog(QString("Loaded example '%1'.").arg(name));

        stateInitialJson = json;
        btnStop->setText("Stop");
        btnStop->setEnabled(false);
        btnForwardOnce->setEnabled(true);
        btnForwardTwice->setEnabled(true);

        // Takes care of dynamic fields and image
        emit worker->updated(state.ant_col, state.ant_row, state.ant_orientation, state.last_step_res, 0);

        QString rulesText = rulesToText(state.rules, state.maxval);

        // Takes care of static fields (fields that don't change during simulation lifetime)
        this->gridDimEdit->setText(QString("%1 x %2").arg(state.grid_width).arg(state.grid_height));
        this->maxvalEdit->setText(QString::number(state.maxval));
        this->rulesEdit->setText(rulesText);
    }
    else {
        addLog(QString("Failed to load example '%1'.").arg(name));

        QString msg;
        QTextStream ts(&msg);
        ts << "Failed to load example '" << name << "'.";
        showErrorDialogListErrors(this, msg, errors);
    }
    return wasRunning;
}

void LangtonSimulatorWindow::startSimulation()
{
    worker->running = true;

    if (!workerThread->isRunning())
        workerThread->start(); // triggers QThread::started

    simulationTimer.restart();
    simulationRunning = true;
    simulationStoppedVoluntarily = false;

    btnStop->setText("Stop");
    btnStop->setEnabled(true);

    btnForwardOnce->setEnabled(false);
    btnForwardTwice->setEnabled(false);
}

void LangtonSimulatorWindow::resumeSimulation()
{
    worker->running = true;

    if (!workerThread->isRunning())
        workerThread->start(); // triggers QThread::started

    simulationTimer.resume();
    simulationRunning = true;
    simulationStoppedVoluntarily = false;

    btnStop->setText("Stop");
    btnStop->setEnabled(true);

    btnForwardOnce->setEnabled(false);
    btnForwardTwice->setEnabled(false);
}

bool LangtonSimulatorWindow::stopSimulation(bool voluntary)
{
    bool wasRunning = simulationRunning;
    if (wasRunning) {
        stopWorker();
        simulationRunning = false;
        simulationStoppedVoluntarily = voluntary;

        btnStop->setText("Stop");
        btnStop->setEnabled(false);

        btnForwardOnce->setEnabled(true);
        btnForwardTwice->setEnabled(true);
    }
    return wasRunning;
}

LangtonSimulatorWindow::LangtonSimulatorWindow(QWidget *parent, QString const &title)
    : QMainWindow(nullptr)
{
    Q_UNUSED(parent);
    setWindowTitle(title);
    setAcceptDrops(true);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QHBoxLayout *mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QSplitter *mainSplitter = new QSplitter(Qt::Horizontal, central);
    mainSplitter->setStyleSheet(R"(
        QSplitter::handle { background-color: lightgray; }
        QSplitter::handle:horizontal { width: 2px; }
        QSplitter::handle:vertical { height: 2px; }
    )");
    mainLayout->addWidget(mainSplitter);

    populateCommonMenuBar(this->menuBar());

    QWidget *leftPanel = new QWidget(this);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(4, 4, 4, 4);
    leftLayout->setSpacing(6);

    QWidget *controlsContainer = new QWidget(leftPanel);
    QFormLayout *controlsForm = new QFormLayout(controlsContainer);

    jsonPicker = new FilePicker("Import state:", "", controlsContainer, false, [this]() {
        if (stopSimulation()) {
            addLog(QString("Simulation stopped (browse)"));
        }
    });
    controlsForm->addRow(jsonPicker);

    {
        QWidget *buttonRow = new QWidget(controlsContainer);
        QHBoxLayout *btnLayout = new QHBoxLayout(buttonRow);
        btnLayout->setContentsMargins(0, 0, 0, 0);
        btnLayout->setSpacing(4);

        btnGenerateRandExample = new QPushButton("Generate random", buttonRow);
        btnGenerateRandExample->setToolTip("Replaces the current state with a randomly generated one according to settings. Right click for settings.");
        btnGenerateRandExample->setContextMenuPolicy(Qt::CustomContextMenu);

        generateRandomPopup = new LangtonGenerateRandomPopup(this);

        connect(btnGenerateRandExample, &QWidget::customContextMenuRequested, this, [this](QPoint const &pos) {
            if (generateRandomPopup->isVisible()) {
                btnGenerateRandExample->close();
                return;
            }
            generateRandomPopup->openAt(btnGenerateRandExample->mapToGlobal(pos));
        });

        connect(btnGenerateRandExample, &QPushButton::clicked, this, [this]() {
            bool wasRunning = stopSimulation();

            delete[] this->state.grid;

            i32 gw = generateRandomPopup->gridWidth();
            i32 gh = generateRandomPopup->gridHeight();
            std::string turns = generateRandomPopup->turnsText().toUtf8().constData();                  // Cannot do .toStdString(), or else: Address Sanitizer Error: Mismatch between allocation and deallocation APIs
            std::string shadeOrder = generateRandomPopup->shadeOrderValue().toUtf8().constData();       // Cannot do .toStdString(), or else: Address Sanitizer Error: Mismatch between allocation and deallocation APIs
            std::string orientations = generateRandomPopup->orientationsText().toUtf8().constData();    // Cannot do .toStdString(), or else: Address Sanitizer Error: Mismatch between allocation and deallocation APIs

            auto rand_state = langton::make_randomized_state({2, 255}, {gw, gw}, {gh, gh}, std::string_view(turns), std::string_view(shadeOrder), std::string_view(orientations));
            this->state = rand_state;

            std::ostringstream state_json;
            std::string fill_string = QString("fill=%1").arg(state.grid[0]).toUtf8().constData(); // Cannot do .toStdString(), or else: Address Sanitizer Error: Mismatch between allocation and deallocation APIs

            bool write_success = langton::print_state_json(
                state_json,
                "",
                fill_string,
                false,
                state.generation,
                state.grid_width,
                state.grid_height,
                state.ant_col,
                state.ant_row,
                state.last_step_res,
                state.ant_orientation,
                state.maxval,
                state.rules
            );
            assert(write_success);

            this->stateInitialJson = state_json.str();

            randExampleCombo->setCurrentIndex(0);

            // Takes care of dynamic fields and image
            emit worker->updated(state.ant_col, state.ant_row, state.ant_orientation, state.last_step_res, 0);

            viewer->center(10);

            QString rulesText = rulesToText(state.rules, state.maxval);

            // Takes care of static fields (fields that don't change during simulation lifetime)
            this->gridDimEdit->setText(QString("%1 x %2").arg(state.grid_width).arg(state.grid_height));
            this->maxvalEdit->setText(QString::number(state.maxval));
            this->rulesEdit->setText(rulesText);

            addLog(QString("Generated random example."));

            if (wasRunning || !simulationStoppedVoluntarily) {
                startSimulation();
                addLog("Simulation started automatically.");
            }
        });

        auto btnLoadRandExample = new QPushButton("Load random example", buttonRow);

        connect(btnLoadRandExample, &QPushButton::clicked, this, [this]() {
            bool wasRunning = loadRandomExample();

            randExampleCombo->setCurrentIndex(0);
            viewer->center(10);

            if (wasRunning || !simulationStoppedVoluntarily) {
                startSimulation();
                addLog("Simulation started automatically.");
            }
        });

        randExampleCombo = new QComboBox(this);
        randExampleCombo->setEditable(false);

        randExampleCombo->addItem("— Select example to load —"); // index 0 = blank state
        for (auto const &example : examples) {
            QString name = QString::fromStdString(std::get<0>(example));
            randExampleCombo->addItem(name);
        }
        randExampleCombo->setCurrentIndex(0);

        connect(randExampleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (index == -1 || index == 0) { // -1 = no selection, 0 = blank state, both represent "no example loaded"
                return;
            }

            auto const &example = examples[index - 1]; // -1 because of blank state at index 0
            bool wasRunning = loadExample(std::get<0>(example), std::get<1>(example));
            viewer->center(10);

            if (wasRunning || !simulationStoppedVoluntarily) {
                startSimulation();
                addLog("Simulation started automatically.");
            }
        });

        btnLayout->addWidget(btnGenerateRandExample);
        btnLayout->addWidget(btnLoadRandExample);
        btnLayout->addWidget(randExampleCombo);

        controlsForm->addRow(buttonRow);
    }

    connect(jsonPicker, &FilePicker::fileChanged, this, [this]() {
        bool wasRunning = stopSimulation();

        QString path = jsonPicker->file();
        bool success = importJsonFile(path);

        if (success) {
            this->iterationEdit->setText("0");
            this->antPosEdit->setText(QString("%1, %2").arg(state.ant_col).arg(state.ant_row));
            this->orientationEdit->setText(langton::orientation::to_cstr(state.ant_orientation));
            this->lastStepResEdit->setText(langton::step_result::to_cstr(state.last_step_res));

            viewer->setBuffer(this->state.grid, this->state.grid_width, this->state.grid_height, this->state.maxval, normalizeCheckbox->isChecked());
            viewer->center(10);
            viewer->refresh();

            if (wasRunning || !simulationStoppedVoluntarily) {
                startSimulation();
                addLog("Simulation started automatically.");
            }
        } else {
            jsonPicker->restorePrevFileSilently();
            if (wasRunning || !simulationStoppedVoluntarily) {
                startSimulation();
                addLog("Simulation resumed automatically.");
            }
        }
    });

    logEdit = new QTextEdit(controlsContainer);
    logEdit->setReadOnly(true);
    logEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    {
        QFont font("Consolas");
        font.setStyleHint(QFont::Monospace);
        font.setFixedPitch(true);
        logEdit->setFont(font);
        logEdit->setLineWrapColumnOrWidth(0);
        logEdit->setWordWrapMode(QTextOption::NoWrap);
    }

    controlsForm->addRow(logEdit);

    auto add_lines = [](u64 quantity, QLayout *l, QWidget *parent) {
        for (u64 i = 0; i < quantity; ++i) {
            QFrame *line = new QFrame(parent);
            l->addWidget(line);
        }
    };

    add_lines(4, controlsForm, controlsContainer);

    {
        rendersPerSecEdit = new QLineEdit(controlsContainer);
        rendersPerSecEdit->setText("0 renders/sec");
        rendersPerSecEdit->setReadOnly(true);

        renderPerSecTimer = new QTimer(this);
        renderPerSecTimer->setInterval(actualTpsTimerIntervalMsec);

        connect(renderPerSecTimer, &QTimer::timeout, this, [this]() {
            u64 current = renderCount;
            u64 rendersDelta = current - lastRenderCount;

            lastRenderCount = current;

            double rps = rendersDelta / (actualTpsTimerIntervalMsec / 1000.0);

            rendersPerSecEdit->setText(QString("%1 renders/sec").arg(QLocale().toString(rps, 'f', 0)));
        });

        renderPerSecTimer->start();

        auto btnCenterImage = new QPushButton("Center Image", controlsContainer);
        btnCenterImage->setDisabled(false);

        connect(btnCenterImage, &QPushButton::clicked, this, [this]() {
            viewer->center(10);
        });

        normalizeCheckbox = new QCheckBox("Normalize image", controlsContainer);
        normalizeCheckbox->setChecked(true);

        connect(normalizeCheckbox, &QCheckBox::toggled, this, [this](bool checked) {
            viewer->setBuffer(this->state.grid, this->state.grid_width, this->state.grid_height, this->state.maxval, checked);
            viewer->refresh();
            ++renderCount;
        });

        QWidget *buttonRow = new QWidget(controlsContainer);
        QHBoxLayout *btnLayout = new QHBoxLayout(buttonRow);
        btnLayout->setContentsMargins(0, 0, 0, 0);
        btnLayout->setSpacing(10);

        btnLayout->addWidget(rendersPerSecEdit);
        btnLayout->addWidget(btnCenterImage);
        btnLayout->addWidget(normalizeCheckbox);

        controlsForm->addRow(buttonRow);
    }
    add_lines(1, controlsForm, controlsContainer);
    {
        simulationTypeCombo = new QComboBox(parent);

        simulationTypeCombo->addItem("Adaptive");
        simulationTypeCombo->setItemData(0, "Simulation speed is throttled (if necessary) to allow a reasonable render rate at high speeds.\nMax simulation speed is reduced compared to 'Unrestricted' mode.\nChange takes effect on next simulation start.", Qt::ToolTipRole);

        simulationTypeCombo->addItem("Unrestricted");
        simulationTypeCombo->setItemData(1, "Prioritizes execution speed over render timeliness.\nRender updates will slow down as desired speed approaches and/or exceeds maximum throughput.\nChange takes effect on next simulation start.", Qt::ToolTipRole);

        controlsForm->addRow("Simulation Type:", simulationTypeCombo);
    }
    {
        actualTpsTimer = new QTimer(this);
        actualTpsTimer->setInterval(actualTpsTimerIntervalMsec);

        actualTpsEdit = new QLineEdit(parent);
        actualTpsEdit->setText("0 ticks/sec");
        actualTpsEdit->setReadOnly(true);

        controlsForm->addRow("Throughput:", actualTpsEdit);

        connect(actualTpsTimer, &QTimer::timeout, this, [this]() {
            u64 current = worker->iterationCount.load();
            u64 iterationsDelta;
            if (current >= lastIterationCount) {
                iterationsDelta = current - lastIterationCount;
            } else {
                // Simulation was restarted, prevent overflow
                iterationsDelta = current;
            }

            lastIterationCount = current;

            double ips = iterationsDelta / (actualTpsTimerIntervalMsec / 1000.0);

            actualTpsEdit->setText(QString("%1 ticks/sec").arg(QLocale().toString(ips, 'f', 0)));
        });

        actualTpsTimer->start();
    }
    {
        FactorSpinBox *tpsSpin = new FactorSpinBox(parent, " ticks/sec");
        tpsSpin->setRange(1, 1e12);
        tpsSpin->setDecimals(0); // integer only
        tpsSpin->setValue(1000);

        controlsForm->addRow("Target:", tpsSpin);

        connect(tpsSpin, QOverload<double>::of(&FactorSpinBox::valueChanged), this, [this](double value){
            iterationsPerSecond = value;
            if (worker) worker->iterationsPerSecond = value;
        });
    }
    add_lines(1, controlsForm, controlsContainer);
    {
        QWidget *buttonRow = new QWidget(controlsContainer);
        QHBoxLayout *btnLayout = new QHBoxLayout(buttonRow);
        btnLayout->setContentsMargins(0, 0, 0, 0);
        btnLayout->setSpacing(4);

        auto make_disabled_pushbutton = [](char const *text, QWidget *parent) -> QPushButton * {
            auto btn = new QPushButton(text, parent);
            btn->setDisabled(true);
            return btn;
        };

        btnBackTwice      = make_disabled_pushbutton("<<",    buttonRow);
        btnBackOnce       = make_disabled_pushbutton("<",     buttonRow);
        btnStop           = make_disabled_pushbutton("Stop",  buttonRow);
        btnForwardOnce        = make_disabled_pushbutton(">",     buttonRow);
        btnForwardTwice   = make_disabled_pushbutton(">>",    buttonRow);

        btnLayout->addWidget(btnBackTwice);
        btnLayout->addWidget(btnBackOnce);
        btnLayout->addWidget(btnStop);
        btnLayout->addWidget(btnForwardOnce);
        btnLayout->addWidget(btnForwardTwice);

        controlsForm->addRow(buttonRow);
    }

    add_lines(2, controlsForm, controlsContainer);

    QScrollArea *controlsScroll = new QScrollArea(leftPanel);
    controlsScroll->setWidgetResizable(true);
    controlsScroll->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    controlsScroll->setWidget(controlsContainer);

    leftLayout->addWidget(controlsScroll, 0);

    connect(btnForwardOnce, &QPushButton::clicked, this, [this]() {
        state.last_step_res = langton::attempt_step_forward(state);

        ++state.generation;
        ++worker->iterationCount;

        if (state.last_step_res != langton::step_result::HIT_EDGE) {
            this->antPosEdit->setText(QString("%1, %2").arg(state.ant_col).arg(state.ant_row));
            this->orientationEdit->setText(langton::orientation::to_cstr(state.ant_orientation));
            this->lastStepResEdit->setText(langton::step_result::to_cstr(state.last_step_res));
            {
                auto current_text = this->iterationEdit->text();
                bool ok = false;
                u64 current_value = current_text.remove(',').toULongLong(&ok);
                this->iterationEdit->setText(QLocale().toString( current_value + 1 ));
            }

            viewer->setBuffer(this->state.grid, this->state.grid_width, this->state.grid_height, this->state.maxval, normalizeCheckbox->isChecked());
            viewer->refresh();
            ++renderCount;
        }
    });

    qRegisterMetaType<quint64>("u64");
    qRegisterMetaType<langton::orientation::value_type>("langton::orientation::value_type");
    qRegisterMetaType<langton::step_result::value_type>("langton::step_result::value_type");

    worker = new LangtonSimulationWorker(&state);
    workerThread = new QThread(this);
    worker->moveToThread(workerThread);

    connect(btnForwardTwice, &QPushButton::clicked, this, [this]() {
        startSimulation();
        addLog("Simulation started by user.");
    });

    connect(btnStop, &QPushButton::clicked, this, [this]() {
        if (btnStop->text() == "Reset") {
            stopWorker();
            simulationRunning = false;

            auto errors = importJsonString(stateInitialJson);
            if (errors.empty()) {
                btnStop->setText("Stop");
                btnStop->setEnabled(false);
                btnForwardOnce->setEnabled(true);
                btnForwardTwice->setEnabled(true);
                addLog(QString("Initial state restored (reset)."));
            }
            else {
                QString msg;
                QTextStream ts(&msg);
                ts << "Failed to restore initial state (reset).";
                addLog(msg);
                showErrorDialogListErrors(this, msg, errors);
            }

            this->iterationEdit->setText("0");
            this->antPosEdit->setText(QString("%1, %2").arg(state.ant_col).arg(state.ant_row));
            this->orientationEdit->setText(langton::orientation::to_cstr(state.ant_orientation));
            this->lastStepResEdit->setText(langton::step_result::to_cstr(state.last_step_res));

            viewer->setBuffer(this->state.grid, this->state.grid_width, this->state.grid_height, this->state.maxval, normalizeCheckbox->isChecked());
            viewer->refresh();
            ++renderCount;
        }
        else {
            assert(simulationRunning);

            if (stopSimulation(true)) {
                f64 sec = f64(simulationTimer.elapsed()) / 1000.0;
                u64 averageTps = worker->iterationCount.load() / sec;
                addLog(QString("Simulation stopped by user after %1 s, avg. %2 TPS.").arg(sec, 0, 'f', 1).arg(averageTps));
            }
        }
    });

    connect(workerThread, &QThread::started, worker, [this]() {
        auto current_text = this->iterationEdit->text();
        bool ok = false;
        u64 currentIterationCount = current_text.remove(',').toULongLong(&ok);

        if (simulationTypeCombo->currentText() == "Unrestricted") { // Unrestricted
            worker->run_forward_unrestricted(currentIterationCount, iterationsPerSecond);
        }
        else {
            worker->run_forward_adaptive(currentIterationCount, iterationsPerSecond, 60);
        }
    });

    connect(worker, &LangtonSimulationWorker::finished, workerThread, &QThread::quit);

    connect(worker, &LangtonSimulationWorker::updated, this, [this](
        u64 col, u64 row,
        langton::orientation::value_type ori,
        langton::step_result::value_type lastStep,
        u64 iterCount
    ) {
        if (lastStep == langton::step_result::HIT_EDGE) {
            f64 sec = f64(simulationTimer.elapsed()) / 1000.0;
            u64 averageTps = worker->iterationCount.load() / sec;

            simulationRunning = false;
            btnStop->setText("Reset");
            btnStop->setEnabled(true);
            btnForwardOnce->setEnabled(false);
            btnForwardTwice->setEnabled(false);

            addLog(QString("Simulation hit edge after %1 s, avg. %2 TPS.").arg(sec, 0, 'f', 1).arg(averageTps));
        }

        iterationEdit->setText(QLocale().toString(iterCount));
        antPosEdit->setText(QString("%1, %2").arg(col).arg(row));
        orientationEdit->setText(langton::orientation::to_cstr(ori));
        lastStepResEdit->setText(langton::step_result::to_cstr(lastStep));

        viewer->setBuffer(this->state.grid, this->state.grid_width, this->state.grid_height, this->state.maxval, normalizeCheckbox->isChecked());
        viewer->update();
        ++renderCount;
    });

    auto make_readonly_QLineEdit = [](QWidget *parent) -> QLineEdit * {
        QLineEdit *le = new QLineEdit(parent);
        le->setReadOnly(true);
        le->setEnabled(true);
        return le;
    };

    gridDimEdit     = make_readonly_QLineEdit(controlsContainer);
    antPosEdit      = make_readonly_QLineEdit(controlsContainer);
    orientationEdit = make_readonly_QLineEdit(controlsContainer);
    lastStepResEdit = make_readonly_QLineEdit(controlsContainer);
    maxvalEdit      = make_readonly_QLineEdit(controlsContainer);
    iterationEdit   = make_readonly_QLineEdit(controlsContainer);

    rulesEdit = new QTextEdit(controlsContainer);
    rulesEdit->setReadOnly(true);
    rulesEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    {
        QFont font("Consolas");
        font.setStyleHint(QFont::Monospace);
        font.setFixedPitch(true);
        rulesEdit->setFont(font);
        rulesEdit->setLineWrapColumnOrWidth(0);
        rulesEdit->setWordWrapMode(QTextOption::NoWrap);
    }

    add_lines(4, controlsForm, controlsContainer);

    controlsForm->addRow("Iteration",         iterationEdit);
    controlsForm->addRow("Ant Pos (x, y)",    antPosEdit);
    controlsForm->addRow("Orientation",       orientationEdit);
    controlsForm->addRow("Last Step Result",  lastStepResEdit);

    add_lines(2, controlsForm, controlsContainer);

    controlsForm->addRow("Grid (w x h)",      gridDimEdit);
    controlsForm->addRow("Max Value",         maxvalEdit);
    controlsForm->addRow("Rules",             rulesEdit);

    mainSplitter->addWidget(leftPanel);

    viewer = new RawGridView(state.grid, state.grid_width, state.grid_height);
    mainSplitter->addWidget(viewer);

    mainSplitter->setStretchFactor(0, 0);
    mainSplitter->setStretchFactor(1, 1);

    loadRandomExample();
}

LangtonSimulatorWindow::~LangtonSimulatorWindow()
{
    stopSimulation();
    delete[] state.grid;
}

void LangtonSimulatorWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        auto urls = event->mimeData()->urls();
        if (!urls.isEmpty()) {
            QString path = urls.first().toLocalFile();
            QFileInfo info(path);
            if (info.isFile()) {
                event->acceptProposedAction();
                return;
            }
        }
    }
    event->ignore();
}

void LangtonSimulatorWindow::dropEvent(QDropEvent *event)
{
    auto urls = event->mimeData()->urls();
    if (!urls.isEmpty()) {
        QString path = urls.first().toLocalFile();
        QFileInfo info(path);
        if (info.isFile()) {
            jsonPicker->setFile(path);
        }
    }
}

void LangtonSimulatorWindow::stopWorker()
{
    if (workerThread && workerThread->isRunning()) {
        worker->running = false;
        workerThread->quit();
        workerThread->wait();  // BLOCK until thread exits
    }
}

std::vector<std::string> LangtonSimulatorWindow::importJsonString(std::string const &contents)
{
    std::vector<std::string> errors = {};
    auto parsed_state = langton::parse_state(contents, {}, errors);

    if (!errors.empty()) {
        return errors;
    }
    // Only overwrite state if parse succeeded:

    delete[] this->state.grid;
    this->state = parsed_state;

    state.maxval = langton::deduce_maxval_from_rules(state.rules);

    return errors;
}

bool LangtonSimulatorWindow::importJsonFile(QString const &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString msg;
        QTextStream ts(&msg);
        ts << "Failed to open file: " << path;
        showErrorDialogSimple(this, msg);
        return false;
    }

    QByteArray bytes = file.readAll();
    std::string contents(bytes.constData(), bytes.size());

    auto errors = importJsonString(contents);

    if (errors.empty()) {
        addLog(QString("Loaded file: %1").arg(path));
        return true;
    }
    else {
        addLog(QString("Failed to load file: %1").arg(path));

        QString msg;
        QTextStream ts(&msg);
        ts << "Failed to parse file: " << path;
        showErrorDialogListErrors(this, msg, errors);
        return false;
    }
}

void LangtonSimulatorWindow::addLog(QString const &entry)
{
    QString currentTime = QDateTime::currentDateTime().toString("HH:mm:ss");
    // QString currentDateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    logEdit->append(QString("[%1] %2").arg(currentTime).arg(entry));
}
