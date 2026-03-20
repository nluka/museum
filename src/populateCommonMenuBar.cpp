#include "LangtonSimulatorWindow.hpp"
#include "CompilerTestsWindow.hpp"
#include "CompilationFlowWindow.hpp"
#include "SwanDockWindow.hpp"

#include "populateCommonMenuBar.hpp"

void populateCommonMenuBar(QMenuBar *menu_bar)
{
    {
        QMenu *window_menu = menu_bar->addMenu("&Window");


        QAction *swan_action = new QAction("New &Swan File Explorer Window", menu_bar);

        window_menu->addAction(swan_action);

        QObject::connect(swan_action, &QAction::triggered, menu_bar, []() {
            SwanDockWindow *w = new SwanDockWindow();
            w->setAttribute(Qt::WA_DeleteOnClose);
            w->resize(1600, 900);
            w->show();
        });


        QAction *langton_action = new QAction("New &Langton's Ant Simulation Window", menu_bar);

        window_menu->addAction(langton_action);

        QObject::connect(langton_action, &QAction::triggered, menu_bar, []() {
            LangtonSimulatorWindow *w = new LangtonSimulatorWindow();
            w->setAttribute(Qt::WA_DeleteOnClose);
            w->resize(1600, 900);
            w->show();
        });


        QAction *empty_compiler_tests_action = new QAction("New Compiler &Tests Window", menu_bar);

        window_menu->addAction(empty_compiler_tests_action);

        QObject::connect(empty_compiler_tests_action, &QAction::triggered, menu_bar, []() {
            QString title = "Compiler Tests";
            CompilerTestsWindow *w = new CompilerTestsWindow(nullptr);
            w->setAttribute(Qt::WA_DeleteOnClose);
            w->resize(1600, 900);
            w->show();
        });

    #if 0
        QAction *empty_compilation_flow_action = new QAction("New &Compilation Flow Window", menu_bar);

        window_menu->addAction(empty_compilation_flow_action);

        QObject::connect(empty_compilation_flow_action, &QAction::triggered, menu_bar, []() {
            QString title = "Compilation Flow";
            CompilationFlowWindow *w = new CompilationFlowWindow(nullptr, title);
            w->setAttribute(Qt::WA_DeleteOnClose);
            w->resize(1600, 900);
            w->show();
        });
    #endif
    }
    {
        QMenu *debug_menu = menu_bar->addMenu("&Debug");

        QAction *ASan_action = new QAction("&Trigger ASan", menu_bar);

        debug_menu->addAction(ASan_action);

        QObject::connect(ASan_action, &QAction::triggered, menu_bar, []() {
            int *p = new int[4];
            p[4] = 123; // intentional heap buffer overflow
        });
    }
}
