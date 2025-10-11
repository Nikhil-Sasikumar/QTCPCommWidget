#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QMetaType>
#include <QSet>
#include <QStandardPaths>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    tcp_server = new QTcpServer();
    if (tcp_server->listen(QHostAddress::Any, 8080)) {
        connect(this, &MainWindow::atNewMessage,
                this, &MainWindow::onDisplayMessage);
        connect(tcp_server, &QTcpServer::newConnection,
                this, &MainWindow::onNewConnection);
        ui->label->setText("Server is listening...");

    } else {
        QMessageBox::critical(this, "QTCPServer", QString("Unable to start the server : %1.").arg(tcp_server->errorString()));
        exit(EXIT_FAILURE);
    }
}

MainWindow::~MainWindow()
{
    foreach (QTcpSocket *tcp_socket, connection_set) {
        tcp_socket->close();
        tcp_socket->deleteLater();
    }

    tcp_server->close();
    tcp_server->deleteLater();

    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    QString receiver = ui->comboBox->currentText();
    if (receiver == "Broadcast") {
        foreach (QTcpSocket *tcp_socket, connection_set)
            SendMessage(tcp_socket);
    } else {
        foreach (QTcpSocket *tcp_socket, connection_set) {
            if (tcp_socket->socketDescriptor() == receiver.toLongLong()) {
                SendMessage(tcp_socket);
                break;
            }
        }
    }

    ui->lineEdit->clear();
}

void MainWindow::on_pushButton_2_clicked()
{
    QString receiver = ui->comboBox->currentText();

    QString file_path = QFileDialog::getOpenFileName(this, ("Select an attachment"),
                                                    QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                                                    ("File (*.json *.txt *.png *.jpg *.jpeg)"));
    if (file_path.isEmpty()) {
        QMessageBox::critical(this,"QTCPClient","You haven't selected any attachment!");
        return;
    }

    if (receiver == "Broadcast") {
        foreach (QTcpSocket *tcp_socket, connection_set)
            SendAttachment(tcp_socket, file_path);
    } else {
        foreach (QTcpSocket *tcp_socket, connection_set) {
            if (tcp_socket->socketDescriptor() == receiver.toLongLong()) {
                SendAttachment(tcp_socket, file_path);
                break;
            }
        }
    }

    ui->lineEdit->clear();
}

void MainWindow::onNewConnection()
{
    while (tcp_server->hasPendingConnections())
        AppendToSocketList(tcp_server->nextPendingConnection());
}

void MainWindow::onReadSocket()
{
    QTcpSocket *tcp_socket = reinterpret_cast<QTcpSocket*>(sender());

    QByteArray buffer;
    QDataStream data_stream(tcp_socket);
    data_stream.setVersion(QDataStream::Qt_5_15);
    data_stream.startTransaction();
    data_stream >> buffer;

    if (!data_stream.commitTransaction()) {
        QString message = QString("%1 :: Waiting for more data to come..").arg(tcp_socket->socketDescriptor());
        emit atNewMessage(message);
        return;
    }

    QString header = buffer.mid(0,128);
    QString file_type = header.split(",")[0].split(":")[1];
    buffer = buffer.mid(128);

    if (file_type == "attachment") {
        QString file_name = header.split(",")[1].split(":")[1];
        QString ext = file_name.split(".")[1];
        QString size = header.split(",")[2].split(":")[1].split(";")[0];

        if (QMessageBox::Yes == QMessageBox::question(this, "QTCPServer",
                                                      QString("You are receiving an attachment from sd:%1 of size: %2 bytes, called %3. Do you want to accept it?")
                                                          .arg(tcp_socket->socketDescriptor()).arg(size).arg(file_name))) {

            QString file_path = QFileDialog::getSaveFileName(this, tr("Save File"),
                                                             QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) +
                                                             "/" + file_name, QString("File (*.%1)").arg(ext));
            QFile file(file_path);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(buffer);
                QString message = QString("INFO :: Attachment from sd:%1 successfully stored on disk under the path %2")
                                      .arg(tcp_socket->socketDescriptor()).arg(QString(file_path));
                emit atNewMessage(message);
            } else {
                QMessageBox::critical(this,"QTCPServer", "An error occurred while trying to write the attachment.");
            }
        } else {
            QString message = QString("INFO :: Attachment from sd:%1 discarded").arg(tcp_socket->socketDescriptor());
            emit atNewMessage(message);
        }

    } else if (file_type == "message") {
        QString message = QString("%1 :: %2").arg(tcp_socket->socketDescriptor()).arg(QString::fromStdString(buffer.toStdString()));
        emit atNewMessage(message);
    }
}

void MainWindow::onDiscardSocket()
{
    QTcpSocket *tcp_socket = reinterpret_cast<QTcpSocket*>(sender());
    QSet<QTcpSocket*>::iterator it = connection_set.find(tcp_socket);
    if (it != connection_set.end()) {
        onDisplayMessage(QString("INFO :: A client has just left the room").arg(tcp_socket->socketDescriptor()));
        connection_set.remove(*it);
    }

    RefreshComboBox();
    tcp_socket->deleteLater();
}

void MainWindow::onDisplayError(QAbstractSocket::SocketError socket_error)
{
    switch (socket_error) {
        case QAbstractSocket::RemoteHostClosedError:
            break;
        case QAbstractSocket::HostNotFoundError:
            QMessageBox::information(this, "QTCPServer", "The host was not found. Please check the host name and port settings.");
            break;
        case QAbstractSocket::ConnectionRefusedError:
            QMessageBox::information(this, "QTCPServer", "The connection was refused by the peer. Make sure QTCPServer is running, "
                                                         "and check that the host name and port settings are correct.");
            break;
        default:
            QTcpSocket *tcp_socket = qobject_cast<QTcpSocket*>(sender());
            QMessageBox::information(this, "QTCPServer", QString("The following error occurred: %1.").arg(tcp_socket->errorString()));
            break;
    }
}

void MainWindow::onDisplayMessage(const QString &str)
{
    ui->textBrowser->append(str);
}

void MainWindow::AppendToSocketList(QTcpSocket *tcp_socket)
{
    connection_set.insert(tcp_socket);
    connect(tcp_socket, &QTcpSocket::readyRead,
            this, &MainWindow::onReadSocket);
    connect(tcp_socket, &QTcpSocket::disconnected,
            this, &MainWindow::onDiscardSocket);
    connect(tcp_socket, &QAbstractSocket::errorOccurred,
            this, &MainWindow::onDisplayError);

    ui->comboBox->addItem(QString::number(tcp_socket->socketDescriptor()));
    onDisplayMessage(QString("INFO :: Client with sockd:%1 has just entered the room").arg(tcp_socket->socketDescriptor()));
}

void MainWindow::SendMessage(QTcpSocket *tcp_socket)
{
    if (tcp_socket) {
        if (tcp_socket->isOpen()) {
            QString str = ui->lineEdit->text();
            QDataStream data_stream(tcp_socket);
            data_stream.setVersion(QDataStream::Qt_5_15);

            QByteArray header;
            header.prepend(QString("fileType:message,fileName:null,fileSize:%1;").arg(str.size()).toUtf8());
            header.resize(128);

            QByteArray byte_array = str.toUtf8();
            byte_array.prepend(header);

            data_stream.setVersion(QDataStream::Qt_5_15);
            data_stream << byte_array;
        } else {
            QMessageBox::critical(this, "QTCPServer", "Socket doesn't seem to be opened");
        }
    } else {
        QMessageBox::critical(this, "QTCPServer", "Not connected");
    }
}

void MainWindow::SendAttachment(QTcpSocket *tcp_socket, QString file_path)
{
    if (tcp_socket) {
        if (tcp_socket->isOpen()) {

            QFile file(file_path);
            if (file.open(QIODevice::ReadOnly)) {
                QFileInfo file_info(file.fileName());
                QString file_name(file_info.fileName());

                QDataStream data_stream(tcp_socket);
                data_stream.setVersion(QDataStream::Qt_5_15);

                QByteArray header;
                header.prepend(QString("fileType:attachment,fileName:%1,fileSize:%2;").arg(file_name).arg(file.size()).toUtf8());
                header.resize(128);

                QByteArray byte_array = file.readAll();
                byte_array.prepend(header);
                data_stream << byte_array;
            } else {
                QMessageBox::critical(this, "QTCPClient", "Couldn't open the attachment!");
            }
        } else {
            QMessageBox::critical(this, "QTCPServer", "Socket doesn't seem to be opened");
        }
    } else {
        QMessageBox::critical(this, "QTCPServer", "Not connected");
    }
}

void MainWindow::RefreshComboBox()
{
    ui->comboBox->clear();
    ui->comboBox->addItem("Broadcast");
    foreach(QTcpSocket *tcp_socket, connection_set)
        ui->comboBox->addItem(QString::number(tcp_socket->socketDescriptor()));
}
