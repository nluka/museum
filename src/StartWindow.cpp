#include <QVBoxLayout>
#include <QKeyEvent>
#include <QLabel>

#include "CompilerTestsWindow.hpp"
#include "CompilationFlowWindow.hpp"
#include "LangtonSimulatorWindow.hpp"
#include "SwanDockWindow.hpp"

#include "StartWindow.hpp"

StartWindow::StartWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout *layout = new QVBoxLayout(central);
    layout->setSpacing(12);
    layout->setContentsMargins(40, 40, 40, 40);

    struct Entry {
        bool enabled;
        QString name;
        QChar key;
        std::function<void()> createWin;
    };

    QVector<Entry> entries = {
        { 1, "   (S)wan [file explorer]   ",             'S', [this](){ openSwan(); } },
        { 1, "   (L)angton's Ant Simulator   ",          'L', [this](){ openLangton(); } },
        { 1, "   Compiler (T)ests   ",                   'T', [this](){ openCompilerTests(); } },
        { 0, "   Compilation (F)low   ",                 'F', [this](){ openCompilationFlow(); } },
        { 0, "   (D)SA   ",                              'D', [this](){ openDSA(); } },
        { 0, "   file(u)til   ",                         'U', [this](){ openFileUtil(); } },
        { 0, "   (k)eyCap   ",                           'K', [this](){ openKeyCap(); } },
        { 0, "   Packet (C)apture Exercise   ",          'C', [this](){ openPacketCapture(); } },
        { 0, "   (P)erformance Aware Programming   ",    'P', [this](){ openPerfAware(); } },
    };

#if 1
    // Keep the pointers so we can handle arrow navigation
    i32 number = 1;

    for (auto &e : entries) {

        QHBoxLayout *row = new QHBoxLayout();
        row->setSpacing(12);

        QLabel *numLabel = new QLabel(this);
        numLabel->setMinimumWidth(24);
        numLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        QPushButton *btn = new QPushButton(e.name, this);
        btn->setMinimumHeight(48);
        btn->setDisabled(!e.enabled);

        if (e.enabled) {
            numLabel->setText(QString::number(number));
            number++;
        } else {
            numLabel->setText("");
        }

        // Add accelerator: "&T", "&F", etc.
        QString text = e.name;
        int idx = text.indexOf('(' + e.key + ')');
        if (idx >= 0)
            text.replace(idx+1, 1, "&" + QString(e.key));
        btn->setText(text);

        row->addWidget(numLabel);
        row->addWidget(btn, 1);

        layout->addLayout(row);

        connect(btn, &QPushButton::clicked, this, [this, e]() {
            e.createWin();
            this->close();
        });

        m_buttons.append(btn);
    }
#else
    for (auto &e : entries) {
        QPushButton *btn = new QPushButton(e.name, this);
        btn->setMinimumHeight(48);
        btn->setDisabled(!e.enabled);

        // Add an accelerator: "&T", "&F", etc.
        QString text = e.name;
        int idx = text.indexOf('(' + e.key + ')');
        if (idx >= 0)
            text.replace(idx+1, 1, "&" + QString(e.key));
        btn->setText(text);

        layout->addWidget(btn);

        // Button click → open window
        connect(btn, &QPushButton::clicked, this, [this, e]() {
            e.createWin();
            this->close();
        });

        m_buttons.append(btn);
    }
#endif

    if (!m_buttons.isEmpty())
        m_buttons.first()->setFocus();
}

void StartWindow::keyPressEvent(QKeyEvent *ev)
{
    //
    // Accelerator keys
    //
    if (!ev->text().isEmpty()) {
        QChar c = ev->text().toUpper().at(0);

        if (c.isDigit()) {
            i32 digit = c.digitValue();
            i32 idx = digit - 1;
            if (idx >= 0 && idx < m_buttons.size() - 1) {
                auto &btn = m_buttons[idx];
                btn->animateClick();
                return;
            }
        }

        for (QPushButton *btn : m_buttons) {
            QString t = btn->text().toUpper();
            if (t.contains("&" + QString(c))) {
                btn->animateClick();
                return;
            }
        }
    }

    //
    // Determine which button is focused
    //
    QPushButton *focused = qobject_cast<QPushButton *>(focusWidget());
    int idx = m_buttons.indexOf(focused);

    //
    // Up/Down/Enter behavior
    //
    if (idx != -1) {
        // ↓ arrow → next (wrap-around)
        if (ev->key() == Qt::Key_Down) {
            int next = (idx + 1) % m_buttons.size();
            m_buttons[next]->setFocus();
            return;
        }

        // ↑ arrow → previous (wrap-around)
        if (ev->key() == Qt::Key_Up) {
            int prev = (idx - 1 < 0 ? m_buttons.size() - 1 : idx - 1);
            m_buttons[prev]->setFocus();
            return;
        }

        // Enter activates current
        if (ev->key() == Qt::Key_Return || ev->key() == Qt::Key_Enter) {
            m_buttons[idx]->click();
            return;
        }
    }

    QMainWindow::keyPressEvent(ev);
}

void StartWindow::openCompilerTests()
{
    CompilerTestsWindow *w = new CompilerTestsWindow();
    w->setAttribute(Qt::WA_DeleteOnClose);
    w->resize(1600, 900);
    w->show();
}

void StartWindow::openCompilationFlow()
{
    CompilationFlowWindow *w = new CompilationFlowWindow();
    w->setAttribute(Qt::WA_DeleteOnClose);
    w->resize(1600, 900);
    w->show();
}

void StartWindow::openDSA()
{
    // DSAWindow *w = new DSAWindow();
    // w->setAttribute(Qt::WA_DeleteOnClose);
    // w->show();
}

void StartWindow::openFileUtil()
{
    // FileUtilWindow *w = new FileUtilWindow();
    // w->setAttribute(Qt::WA_DeleteOnClose);
    // w->show();
}

void StartWindow::openKeyCap()
{
    // KeyCapWindow *w = new KeyCapWindow();
    // w->setAttribute(Qt::WA_DeleteOnClose);
    // w->show();
}

void StartWindow::openLangton()
{
    LangtonSimulatorWindow *w = new LangtonSimulatorWindow();
    w->setAttribute(Qt::WA_DeleteOnClose);
    w->resize(1600, 900);
    w->show();
    w->viewer->center(10);
}

void StartWindow::openPacketCapture()
{
    // PacketCaptureWindow *w = new PacketCaptureWindow();
    // w->setAttribute(Qt::WA_DeleteOnClose);
    // w->show();
}

void StartWindow::openPerfAware()
{
    // PerfAwareWindow *w = new PerfAwareWindow();
    // w->setAttribute(Qt::WA_DeleteOnClose);
    // w->show();
}

void StartWindow::openSwan()
{
    SwanDockWindow *w = new SwanDockWindow();
    w->setAttribute(Qt::WA_DeleteOnClose);
    w->resize(1600, 900);
    w->show();
}
