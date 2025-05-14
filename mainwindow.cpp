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
#include <QGroupBox>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTemporaryFile>
#include <QThread>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    auto *centralWidget = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(centralWidget);

    auto *testGroupBox = new QGroupBox("Test Management", this);
    auto *testButtonLayout = new QHBoxLayout();

    auto *createTestButton = new QPushButton("Create Test", this);
    auto *editTestButton = new QPushButton("Edit Test", this);
    auto *showTestInfoButton = new QPushButton("Show Test Info", this);
    auto *deleteTestButton = new QPushButton("Delete Test", this);

    testButtonLayout->addWidget(createTestButton);
    testButtonLayout->addWidget(editTestButton);
    testButtonLayout->addWidget(showTestInfoButton);
    testButtonLayout->addWidget(deleteTestButton);
    testGroupBox->setLayout(testButtonLayout);

    auto *runGroupBox = new QGroupBox("Execution", this);
    auto *runButtonLayout = new QHBoxLayout();

    auto *compileButton = new QPushButton("Compile and Run", this);
    auto *runWithTestButton = new QPushButton("Compile and Run with Test", this);

    runButtonLayout->addWidget(compileButton);
    runButtonLayout->addWidget(runWithTestButton);
    runGroupBox->setLayout(runButtonLayout);

    mainLayout->addWidget(testGroupBox);
    mainLayout->addWidget(runGroupBox);

    codeEditor = new CodeEditor(this);
    codeEditor->setPlaceholderText("// Enter your C++ code here");
    mainLayout->addWidget(codeEditor);

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

    connect(showTestInfoButton, &QPushButton::clicked, this, [this] {
        QString testFilePath = QFileDialog::getOpenFileName(
            this,
            "Select Test to View Info",
            QCoreApplication::applicationDirPath() + "/tests",
            "JSON Files (*.json)"
            );

        if(!testFilePath.isEmpty()) {
            QFile file(testFilePath);
            if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QMessageBox::critical(this, "Error", "Failed to open test file.");
                return;
            }

            QByteArray data = file.readAll();
            file.close();

            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

            if(parseError.error != QJsonParseError::NoError || !doc.isObject()) {
                QMessageBox::critical(this, "Error", "Invalid JSON format.");
                return;
            }

            QJsonObject obj = doc.object();
            QString description = obj.value("description").toString();

            if(description.isEmpty()) {
                QMessageBox::information(this, "Test Info", "No description found in this test.");
            }
            else {
                QMessageBox::information(this, "Test Info", description);
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
    QString filePath = QFileDialog::getSaveFileName(this, "Save File", QDir::homePath(), "C++ Files (*.cpp)");
    if (filePath.isEmpty()) return;

    QFile saveFile(filePath);
    if (!saveFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Cannot save file.");
        return;
    }
    QTextStream saveStream(&saveFile);
    saveStream << codeEditor->toPlainText();
    saveFile.close();

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
    QString expectedInput = testObj["input"].toString();
    QString expectedOutput = testObj["expected"].toString();

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
    program.setProgram(exePath);
    program.setProcessChannelMode(QProcess::MergedChannels);

    program.start();
    if (!program.waitForStarted()) {
        QMessageBox::critical(this, "Error", "Failed to start program.");
        return;
    }

    if (!expectedInput.isEmpty()) {
        program.write(expectedInput.toUtf8());
    }
    program.closeWriteChannel();

    if (!program.waitForFinished()) {
        QMessageBox::critical(this, "Error", "Program did not finish execution.");
        return;
    }

    QString actualOutput = QString::fromUtf8(program.readAll()).trimmed();
    QString cleanedExpectedOutput = expectedOutput.trimmed();

    if (!cleanedExpectedOutput.isEmpty() && actualOutput != cleanedExpectedOutput) {
        QMessageBox::critical(this, "Test Failed",
                              "Expected output:\n" + cleanedExpectedOutput +
                                  "\n\nActual output:\n" + actualOutput);
        return;
    }

    QMessageBox::information(this, "Test Passed", "The test was passed successfully.");

#ifdef Q_OS_WIN
    QString nativeExePath = QDir::toNativeSeparators(exePath);
    QString command = "cmd";
    QStringList args;
    args << "/C" << "start" << "cmd" << "/K" << nativeExePath;
    QProcess::startDetached(command, args);
#endif
}
