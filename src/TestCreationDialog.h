#ifndef TESTCREATIONDIALOG_H
#define TESTCREATIONDIALOG_H

#include <QDialog>

class QLineEdit;
class QTextEdit;

class TestCreationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TestCreationDialog(QWidget *parent = nullptr);
    explicit TestCreationDialog(const QString &filePath, QWidget *parent = nullptr);

private slots:
    void saveTest();

private:
    void loadTest(const QString &filePath);

    QLineEdit *nameEdit;
    QTextEdit *descriptionEdit;
    QLineEdit *forbiddenEdit;
    QTextEdit *inputEdit;
    QTextEdit *expectedOutputEdit;
};

#endif // TESTCREATIONDIALOG_H
