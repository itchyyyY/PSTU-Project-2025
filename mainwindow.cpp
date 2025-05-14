#include "mainwindow.h"
#include <codeeditor.h>
#include "TestCreationDialog.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProcess>
#include <QFileDialog>
#include <QDir>
#include <QTextStream>
#include <QMessageBox>
#include <QSplitter>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTemporaryFile>

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

    codeEditor = new CodeEditor(this);
    codeEditor->setPlaceholderText("// Enter your C++ code here");

    layout->addWidget(codeEditor);

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

    QString sourcePath = QFileDialog::getSaveFileName(
        this, "Сохранить C++ файл",
        QDir::homePath() + "/program.cpp",
        "C++ файлы (*.cpp)");

    if (sourcePath.isEmpty()) return;

    if (!sourcePath.endsWith(".cpp"))
        sourcePath += ".cpp";

    QFile sourceFile(sourcePath);
    if (!sourceFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось сохранить исходный файл.");
        return;
    }

    QTextStream out(&sourceFile);
    out << code;
    sourceFile.close();

    QString exePath = sourcePath;
    exePath.chop(4);

#ifdef Q_OS_WIN
    exePath += ".exe";
#endif

    QProcess compiler;
    compiler.setProgram("g++");
    compiler.setArguments({sourcePath, "-o", exePath});
    compiler.setProcessChannelMode(QProcess::MergedChannels);
    compiler.start();

    if (!compiler.waitForFinished()) {
        QMessageBox::critical(this, "Ошибка", "Процесс компиляции завис или не завершился.");
        return;
    }

    QString compilerOutput = compiler.readAll();
    if (compiler.exitCode() != 0) {
        QMessageBox::critical(this, "Ошибка компиляции",
                              compilerOutput.isEmpty() ? "Неизвестная ошибка компиляции." : compilerOutput);
        return;
    }

    if (!QFile::exists(exePath)) {
        QMessageBox::critical(this, "Ошибка", "Файл .exe не найден после компиляции.");
        return;
    }

    QString nativeExePath = QDir::toNativeSeparators(exePath);

#ifdef Q_OS_WIN
    QString command = "cmd";
    QStringList args;
    args << "/C" << "start" << "cmd" << "/K" << nativeExePath;

    if (!QProcess::startDetached(command, args)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось запустить исполняемый файл в консоли.");
    }
#endif
}


void MainWindow::compileAndRunWithTest() {
    QString testFilePath = QFileDialog::getOpenFileName(
        this, "Select Test",
        QCoreApplication::applicationDirPath() + "/tests",
        "JSON Files (*.json)");

    if (testFilePath.isEmpty()) return;

    QFile testFile(testFilePath);
    if (!testFile.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Error", "Cannot open test file.");
        return;
    }

    QJsonDocument testDoc = QJsonDocument::fromJson(testFile.readAll());
    QJsonObject testObj = testDoc.object();
    QJsonArray forbidden = testObj["forbidden"].toArray();

    QString code = codeEditor->toPlainText();
    for (const QJsonValue &val : forbidden) {
        QString item = val.toString().trimmed();
        if (code.contains(item)) {
            QMessageBox::critical(this, "Test Failed",
                                  "Usage of forbidden element: '" + item + "'");
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
    sourceFile.close();

    QString sourcePath = sourceFile.fileName();
    QString exePath = sourcePath.left(sourcePath.lastIndexOf('.')) + ".exe";

    QProcess compiler;
    compiler.setProgram("g++");
    compiler.setArguments({sourcePath, "-o", exePath});
    compiler.setProcessChannelMode(QProcess::MergedChannels);
    compiler.start();

    if (!compiler.waitForFinished()) {
        QMessageBox::critical(this, "Error", "Compilation process hanged or didn't finish.");
        return;
    }

    QString compilerOutput = compiler.readAll();
    if (compiler.exitCode() != 0) {
        QMessageBox::critical(this, "Compilation Error",
                              compilerOutput.isEmpty() ? "Unknown compilation error." : compilerOutput);
        return;
    }

    if (!QFile::exists(exePath)) {
        QMessageBox::critical(this, "Error", "Executable file not found after compilation.");
        return;
    }

    QProcess program;
    program.start(exePath);
    if (!program.waitForStarted()) {
        QMessageBox::critical(this, "Error", "Failed to start program.");
        return;
    }

    program.closeWriteChannel();
    program.waitForFinished();

    QString nativeExePath = QDir::toNativeSeparators(exePath);
#ifdef Q_OS_WIN
    QString command = "cmd";
    QStringList args;
    args << "/C" << "start" << "cmd" << "/K" << nativeExePath;
    QProcess::startDetached(command, args);
#endif
}
