#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAbstractSocket>
#include <QTcpSocket>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void atNewMessage(QString message);

private slots:
    void on_pushButton_clicked();
    void on_pushButton_2_clicked();

    void onReadSocket();
    void onDiscardSocket();
    void onDisplayMessage(const QString &str);
    void onDisplayError(QAbstractSocket::SocketError socket_error);

private:
    Ui::MainWindow *ui;

    QTcpSocket *tcp_socket;
};

#endif // MAINWINDOW_H
