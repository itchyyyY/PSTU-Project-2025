#ifndef INPUTDIALOG_H
#define INPUTDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

class InputDialog : public QDialog {
    Q_OBJECT
public:
    explicit InputDialog(QWidget *parent = nullptr);

    QString getInput() const;

private slots:
    void onSubmit();

private:
    QLineEdit *inputEdit;
};

#endif // INPUTDIALOG_H
