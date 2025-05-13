#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>

class QPlainTextEdit;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void compileAndRun();
    void compileAndRunWithTest();

private:
    QPlainTextEdit *codeEditor;
    QPlainTextEdit *outputViewer;
};

#endif // MAINWINDOW_H
