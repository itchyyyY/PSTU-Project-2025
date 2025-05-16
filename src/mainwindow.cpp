#include "mainwindow.h"
#include <codeeditor.h>
#include "TestCreationDialog.h"

#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProcess>
#include <QFileDialog>
#include <QDir>
#include <QMessageBox>
#include <QGroupBox>
#include <QFile>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QThread>
#include <QTimer>
#include <QElapsedTimer>
#include <windows.h>

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


void MainWindow::compileAndRun()
{
    QString code = codeEditor->toPlainText();
    if (code.trimmed().isEmpty()) {
        QMessageBox::warning(this, "Empty code", "Please enter code before running.");
        return;
    }

    QString cppFile = QFileDialog::getSaveFileName(
        this,
        "Save C++ File",
        QDir::homePath() + "/main.cpp",
        "C++ Files (*.cpp)"
        );

    if (cppFile.isEmpty()) {
        return;
    }

    QFile file(cppFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Failed to save file.");
        return;
    }

    QTextStream out(&file);
    out << code;
    file.close();

    QFileInfo fileInfo(cppFile);
    QString folderPath = fileInfo.path();
    QString exeFile = fileInfo.dir().filePath(fileInfo.baseName() + ".exe");

    QProcess compileProcess;
    compileProcess.setWorkingDirectory(folderPath);
    compileProcess.start("g++", QStringList() << cppFile << "-o" << exeFile);

    if (!compileProcess.waitForFinished(10000)) {
        QMessageBox::warning(
            this,
            "Compilation Error",
            "Compilation did not finish in time.\n" + compileProcess.readAllStandardError()
            );
        return;
    }

    QString compileErrors = compileProcess.readAllStandardError();
    if (!compileErrors.isEmpty()) {
        QMessageBox::warning(this, "Compilation Error", compileErrors);
        return;
    }

    if (!QFile::exists(exeFile)) {
        QMessageBox::warning(
            this,
            "Error",
            "File " + exeFile + " was not created!"
            );
        return;
    }

    QString command = QString(
                          "cd /d \"%1\" && "
                          "\"%2\" && "
                          "echo Press Enter to close console... && "
                          "pause > nul && "
                          "exit"
                          ).arg(folderPath, exeFile);

    ShellExecuteA(
        NULL,
        "open",
        "cmd.exe",
        QString("/C %1").arg(command).toLocal8Bit().constData(),
        NULL,
        SW_SHOW
        );
}


void MainWindow::compileAndRunWithTest() {
    QString code = codeEditor->toPlainText();
    if (code.trimmed().isEmpty()) {
        QMessageBox::warning(this, "Пустой код", "Пожалуйста, введите код перед запуском.");
        return;
    }

    // Сохраняем C++ файл
    QString cppFile = QFileDialog::getSaveFileName(
        this,
        "Сохранить C++ файл",
        QDir::homePath() + "/main.cpp",
        "C++ Files (*.cpp)"
        );
    if (cppFile.isEmpty())
        return;

    QFile file(cppFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось сохранить файл.");
        return;
    }

    QTextStream out(&file);
    out << code;
    file.close();

    // Открываем тест
    QString testFile = QFileDialog::getOpenFileName(
        this,
        "Выберите файл теста",
        QCoreApplication::applicationDirPath() + "/tests",
        "JSON Files (*.json)"
        );
    if (testFile.isEmpty())
        return;

    QFile jsonFile(testFile);
    if (!jsonFile.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось открыть файл теста.");
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(jsonFile.readAll());
    QJsonObject obj = doc.object();
    QString testInput = obj["input"].toString();
    QString expectedOutput = obj["expected"].toString().trimmed();
    QJsonArray forbiddenArray = obj["forbidden"].toArray();

    QStringList forbiddenList;
    for (const QJsonValue &val : forbiddenArray)
        forbiddenList << val.toString().trimmed();

    jsonFile.close();

    // Проверка запрещённых конструкций
    for (const QString &keyword : forbiddenList) {
        if (!keyword.isEmpty() && code.contains(keyword, Qt::CaseInsensitive)) {
            QMessageBox::warning(this, "Ошибка", "Код содержит запрещённый элемент: " + keyword);
            return;
        }
    }

    // Компиляция
    QFileInfo fileInfo(cppFile);
    QString folderPath = fileInfo.path();
    QString exeFile = fileInfo.dir().filePath(fileInfo.baseName() + ".exe");

    QProcess compileProcess;
    compileProcess.setWorkingDirectory(folderPath);
    compileProcess.start("g++", QStringList() << cppFile << "-o" << exeFile);

    if (!compileProcess.waitForFinished(5000)) {
        QMessageBox::warning(this, "Ошибка компиляции", compileProcess.readAllStandardError());
        return;
    }

    if (!QFile::exists(exeFile)) {
        QMessageBox::warning(this, "Ошибка", "Файл " + exeFile + " не создан!");
        return;
    }

    // Команда: записать вывод в файл И показать вывод пользователю
    QString outputFilePath = folderPath + "/output.txt";
    QString safeInput = testInput;
    safeInput.replace("\"", "\"\""); // экранирование кавычек

    QString cmd = QString(
                      "cd /d \"%1\" && "
                      "(echo %2 | \"%3\") > \"%4\" & "
                      "echo %2 | \"%3\" && "
                      "echo. && echo Нажмите Enter чтобы закрыть консоль... && pause > nul && exit"
                      ).arg(folderPath)
                      .arg(safeInput)
                      .arg(exeFile)
                      .arg(outputFilePath);

    ShellExecuteA(
        NULL,
        "open",
        "cmd.exe",
        QString("/C %1").arg(cmd).toLocal8Bit().constData(),
        NULL,
        SW_SHOW
        );

    // Чтение вывода и проверка теста (с задержкой)
    QTimer::singleShot(500, this, [=]() mutable {
        QElapsedTimer timer;
        timer.start();

        while (!QFile::exists(outputFilePath)) {
            QThread::msleep(200);
            if (timer.elapsed() > 30000) {
                QMessageBox::warning(this, "Таймаут", "Программа не завершилась за отведённое время.");
                return;
            }
        }

        QFile outFile(outputFilePath);
        if (!outFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::warning(this, "Ошибка", "Не удалось прочитать вывод.");
            return;
        }

        QString actualOutput = QString::fromUtf8(outFile.readAll()).trimmed();
        outFile.close();
        QFile::remove(outputFilePath);

        QString resultMessage;

        if (actualOutput == expectedOutput) {
            resultMessage = "✅ Тест пройден успешно.";
        } else {
            resultMessage = "❌ Тест не пройден.";
            resultMessage += "\n\n🔹 Ожидалось:\n" + expectedOutput;
            resultMessage += "\n\n🔹 Получено:\n" + actualOutput;
        }

        if (!testInput.isEmpty() && !actualOutput.contains(testInput)) {
            resultMessage += "\n\n⚠️ Ввод пользователя не соответствует ожиданиям теста.";
        }

        QMessageBox::information(this, "Результат теста", resultMessage);
    });
}
