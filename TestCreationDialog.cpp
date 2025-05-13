#include "TestCreationDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QMessageBox>
#include <QCoreApplication>

TestCreationDialog::TestCreationDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("Creating a test");
    auto *layout = new QVBoxLayout(this);

    nameEdit = new QLineEdit(this);
    descriptionEdit = new QTextEdit(this);
    forbiddenEdit = new QLineEdit(this);

    layout->addWidget(new QLabel("Test name:"));
    layout->addWidget(nameEdit);

    layout->addWidget(new QLabel("Description:"));
    layout->addWidget(descriptionEdit);

    layout->addWidget(new QLabel("Prohibited elements (separated by commas):"));
    layout->addWidget(forbiddenEdit);

    auto *button = new QPushButton("Save", this);
    layout->addWidget(button);

    connect(button, &QPushButton::clicked, this, &TestCreationDialog::onCreateClicked);
}


TestCreationDialog::TestCreationDialog(const QString &filePath, QWidget *parent)
    : TestCreationDialog(parent)
{
    existingFilePath = filePath;
    loadTestFromFile(filePath);
    setWindowTitle("Editing test");
}


void TestCreationDialog::loadTestFromFile(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Error", "Unable to open test file.");
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject obj = doc.object();

    nameEdit->setText(obj["name"].toString());
    descriptionEdit->setText(obj["description"].toString());

    QJsonArray arr = obj["forbidden"].toArray();
    QStringList forbiddenList;
    for (const QJsonValue &val : arr) {
        forbiddenList << val.toString();
    }
    forbiddenEdit->setText(forbiddenList.join(", "));

    file.close();
}


void TestCreationDialog::onCreateClicked() {
    QString name = nameEdit->text().trimmed();
    QString desc = descriptionEdit->toPlainText().trimmed();
    QStringList forbiddenList = forbiddenEdit->text().split(",", Qt::SkipEmptyParts);

    if (name.isEmpty()) {
        QMessageBox::warning(this, "Error", "Enter a test name.");
        return;
    }

    QJsonObject testJson;
    testJson["name"] = name;
    testJson["description"] = desc;

    QJsonArray forbiddenArray;
    for (const QString &item : forbiddenList)
        forbiddenArray.append(item.trimmed());
    testJson["forbidden"] = forbiddenArray;

    QDir dir(QCoreApplication::applicationDirPath());
    if (!dir.exists("tests"))
        dir.mkdir("tests");

    QString pathToSave = existingFilePath.isEmpty()
                             ? dir.filePath("tests/" + name + ".json")
                             : existingFilePath;

    QFile file(pathToSave);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, "Error", "Failed to create or overwrite test file.");
        return;
    }

    file.write(QJsonDocument(testJson).toJson());
    file.close();

    accept();
}
