#include "codeeditor.h"
#include <QKeyEvent>
#include <QTextCursor>
#include <QTextBlock>

CodeEditor::CodeEditor(QWidget *parent)
    : QPlainTextEdit(parent)
{
    QFont font("Consolas", 12);
    setFont(font);
    setTabStopDistance(4 * QFontMetricsF(font).horizontalAdvance(' '));
}


void CodeEditor::keyPressEvent(QKeyEvent *event) {
    QTextCursor cursor = textCursor();

    if (event->key() == Qt::Key_ParenLeft) { // (
        QPlainTextEdit::keyPressEvent(event);
        insertPlainText(")");
        cursor.movePosition(QTextCursor::Left);
        setTextCursor(cursor);
        return;
    }

    if (event->key() == Qt::Key_BraceLeft) { // {
        QPlainTextEdit::keyPressEvent(event);
        insertPlainText("\n\t\n}");
        cursor.movePosition(QTextCursor::Up);
        cursor.movePosition(QTextCursor::EndOfLine);
        setTextCursor(cursor);
        return;
    }

    if (event->key() == Qt::Key_BracketLeft) { // [
        QPlainTextEdit::keyPressEvent(event);
        insertPlainText("]");
        cursor.movePosition(QTextCursor::Left);
        setTextCursor(cursor);
        return;
    }

    if (event->key() == Qt::Key_QuoteDbl) { // "
        QPlainTextEdit::keyPressEvent(event);
        insertPlainText("\"");
        cursor.movePosition(QTextCursor::Left);
        setTextCursor(cursor);
        return;
    }

    if (event->key() == Qt::Key_Apostrophe) { // '
        QPlainTextEdit::keyPressEvent(event);
        insertPlainText("'");
        cursor.movePosition(QTextCursor::Left);
        setTextCursor(cursor);
        return;
    }

    if (event->key() == Qt::Key_Less) {
        QTextCursor cursor = textCursor();
        QString textBeforeCursor = cursor.block().text().left(cursor.positionInBlock());

        if (textBeforeCursor.trimmed().endsWith("#include")) {
            QPlainTextEdit::keyPressEvent(event);
            insertPlainText(">");
            cursor.movePosition(QTextCursor::Left);
            setTextCursor(cursor);
            return;
        }

        QPlainTextEdit::keyPressEvent(event);
        return;
    }

    QPlainTextEdit::keyPressEvent(event);
}
