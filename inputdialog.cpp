#include "inputdialog.h"

InputDialog::InputDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Enter Input (without commas)");

    QVBoxLayout *layout = new QVBoxLayout(this);

    inputEdit = new QLineEdit(this);
    QPushButton *submitButton = new QPushButton("Submit", this);

    layout->addWidget(inputEdit);
    layout->addWidget(submitButton);

    connect(submitButton, &QPushButton::clicked, this, &InputDialog::onSubmit);
}

QString InputDialog::getInput() const {
    return inputEdit->text();
}

void InputDialog::onSubmit() {
    accept();
}
