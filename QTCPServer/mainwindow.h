#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpServer>
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

    void onNewConnection();
    void onReadSocket();
    void onDiscardSocket();
    void onDisplayError(QAbstractSocket::SocketError socket_error);
    void onDisplayMessage(const QString &str);

private:
    void AppendToSocketList(QTcpSocket *tcp_socket);
    void SendMessage(QTcpSocket *tcp_socket);
    void SendAttachment(QTcpSocket *tcp_socket, QString file_path);
    void RefreshComboBox();

    Ui::MainWindow *ui;
    QTcpServer *tcp_server;
    QSet<QTcpSocket*> connection_set;
};

#endif // MAINWINDOW_H
