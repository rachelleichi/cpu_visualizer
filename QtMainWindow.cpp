#include "QtMainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QString>
#include <QTimer>
#include <QCoreApplication>
#include <thread>
#include <chrono>

QtMainWindow::QtMainWindow(QWidget *parent) : QMainWindow(parent), pc(0), cycleCount(0) {
    setupUI();
    loadProgram();
    cpu.buildLabelMap(program);
    updateRegistersGUI();
    updateFlagsGUI();
    updateMemoryGUI();
    updateStackGUI();
    updatePipelineGUI(FETCH);
    highlightInstruction(FETCH);
}

QtMainWindow::~QtMainWindow() {}

void QtMainWindow::setupUI() {
    QWidget *central = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    // Top row: registers + instructions
    QHBoxLayout *topRow = new QHBoxLayout();

    // Registers Table
    registersTable = new QTableWidget(9, 2, this);
    registersTable->setHorizontalHeaderLabels({"Register","Value"});
    registersTable->verticalHeader()->setVisible(false);
    registersTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    QStringList regNames = {"RAX","RBX","RCX","RDX","RSI","RDI","RSP","RBP","RIP"};
    for(int i=0; i<regNames.size(); i++){
        QTableWidgetItem *regItem = new QTableWidgetItem(regNames[i]);
        regItem->setForeground(QBrush(Qt::white));
        regItem->setBackground(QBrush(QColor(0,0,128))); // navy
        registersTable->setItem(i,0,regItem);

        QTableWidgetItem *valItem = new QTableWidgetItem("0");
        valItem->setForeground(QBrush(Qt::white));
        valItem->setBackground(QBrush(QColor(0,0,128))); // navy
        registersTable->setItem(i,1,valItem);
    }
    registersTable->setStyleSheet("QHeaderView::section { background-color: navy; color: white; }");

    // Instructions Table
    instructionsTable = new QTableWidget(0,4,this);
    instructionsTable->setHorizontalHeaderLabels({"Index","Label","Op","Args"});
    instructionsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    instructionsTable->setStyleSheet("QHeaderView::section { background-color: navy; color: white; }");

    topRow->addWidget(registersTable, 1);
    topRow->addWidget(instructionsTable, 2);

    // Middle row: memory + stack
    QHBoxLayout *midRow = new QHBoxLayout();

    memoryTable = new QTableWidget(16,16,this);
    memoryTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    memoryTable->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    for(int i=0;i<256;i++){
        int row=i/16, col=i%16;
        QTableWidgetItem *memItem = new QTableWidgetItem("0");
        memItem->setForeground(QBrush(Qt::white));
        memItem->setBackground(QBrush(QColor(0,0,128)));
        memoryTable->setItem(row,col,memItem);
    }
    memoryTable->setStyleSheet("QHeaderView::section { background-color: navy; color: white; }");

    stackTable = new QTableWidget(16,2,this);
    stackTable->setHorizontalHeaderLabels({"Address","Value"});
    stackTable->verticalHeader()->setVisible(false);
    stackTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    for(int i=0;i<16;i++){
        for(int j=0;j<2;j++){
            QTableWidgetItem *item = new QTableWidgetItem("-");
            item->setForeground(QBrush(Qt::white));
            item->setBackground(QBrush(QColor(0,0,128)));
            stackTable->setItem(i,j,item);
        }
    }
    stackTable->setStyleSheet("QHeaderView::section { background-color: navy; color: white; }");

    midRow->addWidget(memoryTable,3);
    midRow->addWidget(stackTable,1);

    // Flags and pipeline
    flagsLabel = new QLabel("ZF=0 CF=0 SF=0 OF=0", this);
    flagsLabel->setStyleSheet("QLabel { color: black; font-weight:bold; font-size:14px; }");

    pipelineLabel = new QLabel("Pipeline Stage: FETCH", this);
    pipelineLabel->setStyleSheet("QLabel { color: black; font-weight:bold; font-size:14px; }");

    cycleBar = new QProgressBar(this);
    cycleBar->setRange(0,1000);
    cycleBar->setValue(0);

    // Buttons
    stepButton = new QPushButton("Step", this);
    runButton = new QPushButton("Run", this);
    resetButton = new QPushButton("Reset", this);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(stepButton);
    buttonLayout->addWidget(runButton);
    buttonLayout->addWidget(resetButton);
    buttonLayout->addWidget(cycleBar);

    mainLayout->addLayout(topRow);
    mainLayout->addLayout(midRow);
    mainLayout->addWidget(flagsLabel);
    mainLayout->addWidget(pipelineLabel);
    mainLayout->addLayout(buttonLayout);

    setCentralWidget(central);

    // Connect signals
    connect(stepButton, &QPushButton::clicked, this, &QtMainWindow::stepInstruction);
    connect(runButton, &QPushButton::clicked, this, &QtMainWindow::runProgram);
    connect(resetButton, &QPushButton::clicked, this, &QtMainWindow::resetProgram);
}

void QtMainWindow::loadProgram(){
    program = {
        Instruction("start","MOV","RAX","5"),
        Instruction("","MOV","RBX","3"),
        Instruction("","MUL","RAX","RBX"),
        Instruction("","INC","RAX",""),
        Instruction("","CMP","RAX","16"),
        Instruction("","JE","equal",""),
        Instruction("","DEC","RAX",""),
        Instruction("","DIV","RBX",""),
        Instruction("","AND","RAX","RBX"),
        Instruction("","OR","RCX","RAX"),
        Instruction("","XOR","RDX","RBX"),
        Instruction("equal","MOV","RCX","999")
    };

    pc = 0;
    cycleCount = 0;

    instructionsTable->setRowCount(program.size());
    for(int i = 0; i < static_cast<int>(program.size()); ++i){
        QTableWidgetItem *item0 = new QTableWidgetItem(QString::number(i));
        item0->setForeground(QBrush(Qt::white));
        item0->setBackground(QBrush(QColor(0,0,128)));
        instructionsTable->setItem(i,0,item0);

        QTableWidgetItem *item1 = new QTableWidgetItem(QString::fromStdString(program[i].label));
        item1->setForeground(QBrush(Qt::white));
        item1->setBackground(QBrush(QColor(0,0,128)));
        instructionsTable->setItem(i,1,item1);

        QTableWidgetItem *item2 = new QTableWidgetItem(QString::fromStdString(program[i].op));
        item2->setForeground(QBrush(Qt::white));
        item2->setBackground(QBrush(QColor(0,0,128)));
        instructionsTable->setItem(i,2,item2);

        string args = program[i].arg1;
        if(!program[i].arg2.empty()) args += " " + program[i].arg2;
        QTableWidgetItem *item3 = new QTableWidgetItem(QString::fromStdString(args));
        item3->setForeground(QBrush(Qt::white));
        item3->setBackground(QBrush(QColor(0,0,128)));
        instructionsTable->setItem(i,3,item3);
    }
}

void QtMainWindow::updateRegistersGUI(){
    for(int i=0;i<8;++i){ // RAX..RBP
        string reg = registersTable->item(i,0)->text().toStdString();
        registersTable->item(i,1)->setText(QString::number(cpu.getRegister(reg)));
    }
    registersTable->item(8,1)->setText(QString::number(pc)); // RIP
}

void QtMainWindow::updateFlagsGUI(){
    QString text;
    vector<string> fs = {"ZF","CF","SF","OF"};
    for(auto &f : fs){
        if(cpu.getFlag(f)) text += "<b>" + QString::fromStdString(f + "=1 ") + "</b>";
        else text += QString::fromStdString(f + "=0 ");
    }
    flagsLabel->setText(text);
}

void QtMainWindow::updateMemoryGUI(){
    for(int i=0;i<256;++i){
        int row=i/16,col=i%16;
        memoryTable->item(row,col)->setText(QString::number(cpu.getMemory(i)));
    }
}

void QtMainWindow::updateStackGUI(){
    uint64_t rsp = cpu.getRegister("RSP");
    for(int i=0;i<16;++i){
        uint64_t addr = rsp + i;
        if(addr < 256){
            stackTable->item(i,0)->setText(QString::number(addr));
            stackTable->item(i,1)->setText(QString::number(cpu.getMemory(addr)));
        } else {
            stackTable->item(i,0)->setText("-");
            stackTable->item(i,1)->setText("-");
        }
    }
}

void QtMainWindow::updatePipelineGUI(PipelineStage stage){
    QString stageName;
    switch(stage){
    case FETCH: stageName="FETCH"; break;
    case DECODE: stageName="DECODE"; break;
    case EXECUTE: stageName="EXECUTE"; break;
    case WRITEBACK: stageName="WRITEBACK"; break;
    }
    pipelineLabel->setText("Pipeline Stage: " + stageName);
    cycleBar->setValue(static_cast<int>(cycleCount % 1000));
}

void QtMainWindow::highlightInstruction(PipelineStage stage){
    for(int i=0;i<instructionsTable->rowCount();++i){
        for(int j=0;j<instructionsTable->columnCount();++j){
            if(instructionsTable->item(i,j)){
                instructionsTable->item(i,j)->setBackground(QColor(0,0,128));
                instructionsTable->item(i,j)->setForeground(Qt::white);
            }
        }
    }

    if(pc < program.size()){
        QColor color;
        switch(stage){
        case FETCH: color=QColor(0,128,255); break;
        case DECODE: color=QColor(144,238,144); break;
        case EXECUTE: color=QColor(255,255,153); break;
        case WRITEBACK: color=QColor(255,204,153); break;
        }
        for(int j=0;j<instructionsTable->columnCount();++j){
            if(instructionsTable->item(pc,j)){
                instructionsTable->item(pc,j)->setBackground(color);
                instructionsTable->item(pc,j)->setForeground(Qt::white);
            }
        }
    }
}

void QtMainWindow::stepInstruction(){
    if(pc >= program.size()) return;

    updatePipelineGUI(FETCH);
    highlightInstruction(FETCH);
    QCoreApplication::processEvents();
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    cycleCount++;

    updatePipelineGUI(DECODE);
    highlightInstruction(DECODE);
    QCoreApplication::processEvents();
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    cycleCount++;

    updatePipelineGUI(EXECUTE);
    highlightInstruction(EXECUTE);
    QCoreApplication::processEvents();
    try{
        cpu.execute(program[pc], pc);
    } catch(const std::exception &e){
        setWindowTitle("Runtime error: " + QString::fromStdString(e.what()));
        return;
    }
    updateRegistersGUI();
    updateFlagsGUI();
    updateMemoryGUI();
    updateStackGUI();
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    cycleCount++;

    updatePipelineGUI(WRITEBACK);
    highlightInstruction(WRITEBACK);
    QCoreApplication::processEvents();
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    cycleCount++;

    updatePipelineGUI(FETCH);
    highlightInstruction(FETCH);
    updateRegistersGUI();
    updateFlagsGUI();
    updateMemoryGUI();
    updateStackGUI();
}

void QtMainWindow::runProgram(){
    while(pc < program.size()){
        stepInstruction();
    }
}

void QtMainWindow::resetProgram(){
    cpu = CPU();
    loadProgram();
    cpu.buildLabelMap(program);
    pc = 0;
    cycleCount = 0;
    updateRegistersGUI();
    updateFlagsGUI();
    updateMemoryGUI();
    updateStackGUI();
    updatePipelineGUI(FETCH);
    highlightInstruction(FETCH);
}
