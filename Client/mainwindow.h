#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "protocol.h"

#include <QFile>
#include <QMainWindow>
#include <QString>
#include <cstdint>

class QTcpSocket;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void on_btn_close_clicked();
    void on_btn_sendMsg_clicked();
    void on_btn_login_clicked();
    void on_btn_register_clicked();
    void on_btn_sendFile_clicked();
    void on_btn_downloadFile_clicked();
    void on_btn_connectServer_clicked();

    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onSocketError();

private:
    void appendChatMessage(const QString &sender, const QString &message);
    void appendSystemLine(const QString &message);
    QString chatHeaderColor(const QString &sender) const;
    void sendPacket(std::uint16_t cmd, const std::string &payload);
    void updateButtonStates();
    void processPacket(const Protocol::Packet &packet);
    bool ensureLoggedIn();
    void closeTransferFiles();
    QString serverHost() const;
    quint16 serverPort() const;

    Ui::MainWindow *ui;
    QTcpSocket *m_socket;
    QByteArray m_buffer;
    QString m_username;
    QFile m_downloadFile;
    QString m_downloadFileName;
    quint64 m_downloadSize = 0;
    quint64 m_downloadReceived = 0;
};

#endif // MAINWINDOW_H
