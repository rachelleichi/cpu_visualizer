#ifndef QTMAINWINDOW_H
#define QTMAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QProgressBar>
#include <vector>
#include "CPU.h"

using namespace std;

class QtMainWindow : public QMainWindow {
    Q_OBJECT

public:
    QtMainWindow(QWidget *parent=nullptr);
    ~QtMainWindow();

private:
    CPU cpu;

    // UI elements
    QTableWidget *registersTable;
    QTableWidget *memoryTable;
    QLabel *flagsLabel;
    QTableWidget *instructionsTable;
    QTableWidget *stackTable;
    QLabel *pipelineLabel;
    QProgressBar *cycleBar;

    QPushButton *stepButton;
    QPushButton *runButton;
    QPushButton *resetButton;

    vector<Instruction> program;
    size_t pc;
    size_t cycleCount;

    void setupUI();
    void loadProgram();
    void updateRegistersGUI();
    void updateFlagsGUI();
    void updateMemoryGUI();
    void updateStackGUI();
    void updatePipelineGUI(PipelineStage stage);
    void highlightInstruction(PipelineStage stage);

private slots:
    void stepInstruction();
    void runProgram();
    void resetProgram();
};

#endif // QTMAINWINDOW_H
