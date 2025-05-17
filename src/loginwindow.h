#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QCoreApplication>

class LoginWindow : public QDialog {
    Q_OBJECT

public:
    explicit LoginWindow(QWidget *parent = nullptr);
    bool isAuthenticated() const { return authenticated; }

private slots:
    void onLoginClicked();

private:
    QLineEdit *usernameLineEdit;
    QLineEdit *passwordLineEdit;
    QPushButton *loginButton;
    bool authenticated = false;

    bool checkCredentials(const QString &username, const QString &password);
};

#endif // LOGINWINDOW_H
