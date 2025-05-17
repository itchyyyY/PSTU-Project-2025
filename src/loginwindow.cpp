#include "loginwindow.h"

LoginWindow::LoginWindow(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Авторизация");
    setFixedSize(300, 150);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    usernameLineEdit = new QLineEdit(this);
    usernameLineEdit->setPlaceholderText("Логин");
    usernameLineEdit->move(50, 20);
    usernameLineEdit->resize(200, 25);

    passwordLineEdit = new QLineEdit(this);
    passwordLineEdit->setPlaceholderText("Пароль");
    passwordLineEdit->setEchoMode(QLineEdit::Password);
    passwordLineEdit->move(50, 60);
    passwordLineEdit->resize(200, 25);

    loginButton = new QPushButton("Войти", this);
    loginButton->move(100, 100);
    loginButton->resize(100, 30);

    connect(loginButton, &QPushButton::clicked, this, &LoginWindow::onLoginClicked);
}


void LoginWindow::onLoginClicked() {
    QString username = usernameLineEdit->text();
    QString password = passwordLineEdit->text();

    if (checkCredentials(username, password)) {
        authenticated = true;
        accept();
    } else {
        QMessageBox::warning(this, "Ошибка", "Неверный логин или пароль");
    }
}


bool LoginWindow::checkCredentials(const QString &username, const QString &password) {
    QString authPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/auth.txt");

    qDebug() << "Ищем файл авторизации по пути:" << authPath;
    qDebug() << "Существует ли файл:" << QFile::exists(authPath);

    QFile authFile(authPath);

    if (!authFile.exists()) {
        QMessageBox::critical(this, "Ошибка",
                              QString("Файл авторизации не найден!\nОжидаемый путь: %1").arg(authPath));
        return false;
    }

    if (!authFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Ошибка",
                              QString("Не удалось открыть файл авторизации!\nОшибка: %1").arg(authFile.errorString()));
        return false;
    }

    QTextStream in(&authFile);
    QString line = in.readLine().trimmed();
    authFile.close();

    QStringList parts = line.split(":");
    if (parts.size() != 2) {
        QMessageBox::critical(this, "Ошибка",
                              "Некорректный формат файла авторизации!\nОжидается: логин:пароль");
        return false;
    }

    if (username != parts[0] || password != parts[1]) {
        qDebug() << "Введенные данные:" << username << ":" << password;
        qDebug() << "Ожидаемые данные:" << parts[0] << ":" << parts[1];
        return false;
    }

    return true;
}
