#ifndef TESTCREATIONDIALOG_H
#define TESTCREATIONDIALOG_H

#include <QDialog>

class QLineEdit;
class QTextEdit;

class TestCreationDialog : public QDialog {
    Q_OBJECT

public:
    explicit TestCreationDialog(QWidget *parent = nullptr);
    explicit TestCreationDialog(const QString &filePath, QWidget *parent = nullptr);

private:
    QLineEdit *nameEdit;
    QTextEdit *descriptionEdit;
    QLineEdit *forbiddenEdit;
    QTextEdit *inputEdit;
    QTextEdit *expectedOutputEdit;

    QString existingFilePath;

    void saveTestToFile();
    void loadTestFromFile(const QString &filePath);

private slots:
    void onCreateClicked();
};

#endif // TESTCREATIONDIALOG_H
