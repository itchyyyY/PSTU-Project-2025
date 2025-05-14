#include "mainwindow.h"
#include <codeeditor.h>
#include "TestCreationDialog.h"
#include "inputdialog.h"
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProcess>
#include <QTemporaryFile>
#include <QDir>
#include <QTextStream>
#include <QMessageBox>
#include <QSplitter>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QLabel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    auto *centralWidget = new QWidget(this);
    auto *layout = new QVBoxLayout(centralWidget);

    auto *buttonLayout = new QHBoxLayout();

    auto *createTestButton = new QPushButton("Create test", this);
    auto *compileButton = new QPushButton("Compile and Run", this);
    auto *runWithTestButton = new QPushButton("Compile and Run with Test", this);
    auto *editTestButton = new QPushButton("Edit test", this);
    auto *deleteTestButton = new QPushButton("Delete test", this);

    buttonLayout->addWidget(createTestButton);
    buttonLayout->addWidget(editTestButton);
    buttonLayout->addWidget(deleteTestButton);
    buttonLayout->addWidget(compileButton);
    buttonLayout->addWidget(runWithTestButton);

    layout->addLayout(buttonLayout);

    auto *splitter = new QSplitter(Qt::Vertical, this);

    codeEditor = new CodeEditor(this);
    codeEditor->setPlaceholderText("// Enter your C++ code here");

    outputViewer = new CodeEditor(this);
    outputViewer->setReadOnly(true);

    splitter->addWidget(codeEditor);
    splitter->addWidget(outputViewer);

    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);
    splitter->setCollapsible(0, false);
    splitter->setCollapsible(1, false);

    layout->addWidget(splitter);

    connect(compileButton, &QPushButton::clicked, this, &MainWindow::compileAndRun);
    connect(runWithTestButton, &QPushButton::clicked, this, &MainWindow::compileAndRunWithTest);

    connect(createTestButton, &QPushButton::clicked, this, [] {
        TestCreationDialog dialog;
        dialog.exec();
    });

    connect(editTestButton, &QPushButton::clicked, this, [this] {
        QString testFilePath = QFileDialog::getOpenFileName(
            this,
            "Open Test to Edit",
            QCoreApplication::applicationDirPath() + "/tests",
            "JSON Files (*.json)"
            );

        if (!testFilePath.isEmpty()) {
            TestCreationDialog dialog(testFilePath, this);
            dialog.exec();
        }
    });

    connect(deleteTestButton, &QPushButton::clicked, this, [this] {
        QString testFilePath = QFileDialog::getOpenFileName(
            this,
            "Select Test to Delete",
            QCoreApplication::applicationDirPath() + "/tests",
            "JSON Files (*.json)"
            );

        if (!testFilePath.isEmpty()) {
            QFileInfo fileInfo(testFilePath);
            QString fileName = fileInfo.fileName();

            QMessageBox::StandardButton reply = QMessageBox::question(
                this,
                "Confirm Deletion",
                "Delete test \"" + fileName + "\"?",
                QMessageBox::Yes | QMessageBox::No
                );

            if (reply == QMessageBox::Yes) {
                if (QFile::remove(testFilePath)) {
                    QMessageBox::information(this, "Success", "Test deleted.");
                } else {
                    QMessageBox::critical(this, "Error", "Failed to delete test.");
                }
            }
        }
    });

    setCentralWidget(centralWidget);
    setWindowTitle("Project");
    resize(800, 600);
}


MainWindow::~MainWindow() = default;


void MainWindow::compileAndRun() {
    QString code = codeEditor->toPlainText();

    QString tempDir = "C:/Temp";
    QDir dir(tempDir);
    if (!dir.exists()) {
        dir.mkpath(tempDir);
    }

    QString basePath = tempDir + "/qt_temp_code";
    QString sourcePath = basePath + ".cpp";
    QString executablePath = basePath + ".exe";

    QFile sourceFile(sourcePath);
    if (!sourceFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Failed to create source file.");
        return;
    }

    QTextStream out(&sourceFile);
    out << code;
    sourceFile.close();

    QProcess compiler;
    compiler.start("g++", {sourcePath, "-o", executablePath});
    compiler.waitForFinished();

    QString compileErrors = compiler.readAllStandardError();
    if (!compileErrors.isEmpty()) {
        outputViewer->setPlainText("Compilation errors:\n" + compileErrors);
        return;
    }

    if (!QFile::exists(executablePath)) {
        outputViewer->setPlainText("Executable file was not created.");
        return;
    }

    // Проверяем наличие cin в коде
    if (code.contains("cin", Qt::CaseInsensitive)) {
        InputDialog inputDialog(this);
        if (inputDialog.exec() == QDialog::Accepted) {
            QString userInput = inputDialog.getInput();

            QProcess program;
            program.setProcessChannelMode(QProcess::MergedChannels);
            program.start(executablePath);

            if (!program.waitForStarted()) {
                outputViewer->setPlainText("Failed to start program.");
                return;
            }

            program.write(userInput.toUtf8());
            program.closeWriteChannel();
            program.waitForFinished();

            QString output = program.readAllStandardOutput();
            outputViewer->setPlainText("Execution result:\n" + output);
        }
    } else {
        QProcess program;
        program.setProcessChannelMode(QProcess::MergedChannels);
        program.start(executablePath);

        if (!program.waitForStarted()) {
            outputViewer->setPlainText("Failed to start program.");
            return;
        }

        program.waitForFinished();

        QString output = program.readAllStandardOutput();
        outputViewer->setPlainText("Execution result:\n" + output);
    }
}


void MainWindow::compileAndRunWithTest() {
    QString testFilePath = QFileDialog::getOpenFileName(this, "Select Test", QCoreApplication::applicationDirPath() + "/tests", "JSON Files (*.json)");
    if (testFilePath.isEmpty()) return;

    QFile testFile(testFilePath);
    if (!testFile.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Error", "Cannot open test file.");
        return;
    }

    QJsonDocument testDoc = QJsonDocument::fromJson(testFile.readAll());
    QJsonObject testObj = testDoc.object();
    QJsonArray forbidden = testObj["forbidden"].toArray();
    QString inputData = testObj["input"].toString();
    QString expectedOutput = testObj["expected_output"].toString().trimmed();

    QString code = codeEditor->toPlainText();
    for (const QJsonValue &val : forbidden) {
        QString item = val.toString().trimmed();
        if (code.contains(item)) {
            outputViewer->setPlainText("Test failed: usage of forbidden element '" + item + "'");
            return;
        }
    }

    QTemporaryFile sourceFile(QDir::tempPath() + "/temp_code_XXXXXX.cpp");
    if (!sourceFile.open()) {
        QMessageBox::critical(this, "Error", "Failed to create temporary file.");
        return;
    }

    QTextStream out(&sourceFile);
    out << code;
    out.flush();
    QString sourcePath = sourceFile.fileName();
    QString executablePath = sourcePath + ".exe";

    QProcess compiler;
    compiler.start("g++", {sourcePath, "-o", executablePath});
    compiler.waitForFinished();

    QString compileErrors = compiler.readAllStandardError();
    if (!compileErrors.isEmpty()) {
        outputViewer->setPlainText("Compilation errors:\n" + compileErrors);
        return;
    }

    QProcess program;
    program.setProcessChannelMode(QProcess::MergedChannels);
    program.start(executablePath);
    if (!program.waitForStarted()) {
        outputViewer->setPlainText("Failed to start program.");
        return;
    }

    program.write(inputData.toUtf8());
    program.closeWriteChannel();
    program.waitForFinished();

    QString actualOutput = program.readAllStandardOutput().trimmed();

    if (actualOutput == expectedOutput) {
        outputViewer->setPlainText("Test passed!\nOutput:\n" + actualOutput);
    } else {
        outputViewer->setPlainText("Test failed!\nExpected:\n" + expectedOutput + "\n\nActual:\n" + actualOutput);
    }
}
