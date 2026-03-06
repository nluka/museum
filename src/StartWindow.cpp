#include <QVBoxLayout>
#include <QKeyEvent>

#include "CompilerTestsWindow.hpp"
#include "CompilationFlowWindow.hpp"
#include "LangtonSimulatorWindow.hpp"

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
        { true,  "   Compiler (T)ests   ",                   'T', [this](){ openCompilerTests(); } },
        { true,  "   Compilation (F)low   ",                 'F', [this](){ openCompilationFlow(); } },
        { true,  "   (L)angton's Ant Simulator   ",          'L', [this](){ openLangton(); } },
        { false, "   (D)SA   ",                              'D', [this](){ openDSA(); } },
        { false, "   file(u)til   ",                         'U', [this](){ openFileUtil(); } },
        { false, "   (k)eyCap   ",                           'K', [this](){ openKeyCap(); } },
        { false, "   Packet (C)apture Exercise   ",          'C', [this](){ openPacketCapture(); } },
        { false, "   (P)erformance Aware Programming   ",    'P', [this](){ openPerfAware(); } },
        { false, "   (S)wan [file explorer]   ",             'S', [this](){ openSwan(); } },
    };

    // Keep the pointers so we can handle arrow navigation
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

    if (!m_buttons.isEmpty())
        m_buttons.first()->setFocus();
}

void StartWindow::keyPressEvent(QKeyEvent *ev)
{
    //
    // Accelerator keys: T, F, D, U, K, L, C, P, S
    //
    if (!ev->text().isEmpty()) {
        QChar c = ev->text().toUpper().at(0);

        for (QPushButton *btn : m_buttons) {
            QString t = btn->text().toUpper();
            if (t.contains("&" + QString(c))) {
                btn->click();
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
    // SwanFileExplorer *w = new SwanFileExplorer();
    // w->setAttribute(Qt::WA_DeleteOnClose);
    // w->show();
}
