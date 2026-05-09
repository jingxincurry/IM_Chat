#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QDateTime>
#include <QFileDialog>
#include <QFileInfo>
#include <QLineEdit>
#include <QListWidgetItem>
#include <QTextCursor>
#include <QTcpSocket>

namespace {

constexpr qint64 kFileChunkSize = 4096;

std::string toStdString(const QString &text)
{
    const QByteArray bytes = text.toUtf8();
    return std::string(bytes.constData(), static_cast<std::size_t>(bytes.size()));
}

QString fromStdString(const std::string &text)
{
    return QString::fromUtf8(text.data(), static_cast<int>(text.size()));
}

}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_socket(new QTcpSocket(this))
{
    ui->setupUi(this);

    connect(m_socket, &QTcpSocket::connected, this, &MainWindow::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &MainWindow::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &MainWindow::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &MainWindow::onSocketError);

    ui->lineEdit_password->setEchoMode(QLineEdit::Password);
    appendSystemLine(QString::fromUtf8("提示：先连接服务器，再登录。"));
    updateButtonStates();
}

MainWindow::~MainWindow()
{
    closeTransferFiles();
    delete ui;
}

void MainWindow::on_btn_close_clicked()
{
    close();
}

void MainWindow::on_btn_sendMsg_clicked()
{
    if (!ensureLoggedIn()) {
        return;
    }

    const QString content = ui->textEdit_input->toPlainText().trimmed();
    if (content.isEmpty()) {
        appendSystemLine(QString::fromUtf8("提示：消息不能为空。"));
        return;
    }

    sendPacket(Protocol::CmdChatMessage, Protocol::makeStringPayload(toStdString(content)));
    ui->textEdit_input->clear();
}

void MainWindow::on_btn_login_clicked() //登录
{
    const QString username = ui->lineEdit_username->text().trimmed();
    if (username.isEmpty()) {
        appendSystemLine(QString::fromUtf8("提示：用户名不能为空。"));
        return;
    }

    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        appendSystemLine(QString::fromUtf8("提示：请先连接服务器。"));
        return;
    }

    sendPacket(Protocol::CmdLogin, Protocol::makeStringPayload(toStdString(username)));
}

void MainWindow::on_btn_register_clicked()  //注册
{
    on_btn_login_clicked();
}

void MainWindow::on_btn_sendFile_clicked()
{
    if (!ensureLoggedIn()) {
        return;
    }

    const QString filePath = QFileDialog::getOpenFileName(this, QString::fromUtf8("选择要上传的文件"));
    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        appendSystemLine(QString::fromUtf8("错误：无法打开文件。"));
        return;
    }

    const QFileInfo info(file);
    const QString fileName = info.fileName();
    sendPacket(Protocol::CmdFileUploadBegin,
               Protocol::makeFileBeginPayload(toStdString(fileName),
                                              static_cast<Protocol::QWord>(file.size())));

    while (!file.atEnd()) {
        const QByteArray chunk = file.read(kFileChunkSize);
        if (chunk.isEmpty() && file.error() != QFile::NoError) {
            appendSystemLine(QString::fromUtf8("错误：读取文件失败。"));
            file.close();
            return;
        }

        sendPacket(Protocol::CmdFileUploadChunk,
                   std::string(chunk.constData(), static_cast<std::size_t>(chunk.size())));
    }

    file.close();
    sendPacket(Protocol::CmdFileUploadEnd, std::string());
    appendSystemLine(QString::fromUtf8("系统：已发送上传请求：") + fileName);
}

void MainWindow::on_btn_downloadFile_clicked()
{
    if (!ensureLoggedIn()) {
        return;
    }

    QListWidgetItem *item = ui->listWidget_files->currentItem();
    if (item == nullptr) {
        appendSystemLine(QString::fromUtf8("提示：请先选择要下载的文件。"));
        return;
    }

    const QString fileName = item->text();
    const QString savePath = QFileDialog::getSaveFileName(this, QString::fromUtf8("保存文件"), fileName);
    if (savePath.isEmpty()) {
        return;
    }

    closeTransferFiles();
    m_downloadFile.setFileName(savePath);
    if (!m_downloadFile.open(QIODevice::WriteOnly)) {
        appendSystemLine(QString::fromUtf8("错误：无法创建本地文件。"));
        return;
    }

    m_downloadFileName = fileName;
    m_downloadSize = 0;
    m_downloadReceived = 0;
    sendPacket(Protocol::CmdFileDownloadRequest, Protocol::makeStringPayload(toStdString(fileName)));
    appendSystemLine(QString::fromUtf8("系统：开始下载文件：") + fileName);
}

void MainWindow::on_btn_connectServer_clicked()  //连接服务器按钮
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        appendSystemLine(QString::fromUtf8("提示：已经连接到服务器。"));
        return;
    }

    appendSystemLine(QString::fromUtf8("正在连接 123.207.62.1:8888 ..."));
    m_socket->connectToHost("123.207.62.1", 8888);
    updateButtonStates();
}

void MainWindow::onConnected()
{
    appendSystemLine(QString::fromUtf8("系统：服务器连接成功。"));
    updateButtonStates();
}

void MainWindow::onDisconnected()
{
    appendSystemLine(QString::fromUtf8("系统：与服务器断开连接。"));
    m_buffer.clear();
    m_username.clear();
    ui->listWidget_users->clear();
    ui->listWidget_files->clear();
    closeTransferFiles();
    updateButtonStates();
}

void MainWindow::onReadyRead()
{
    m_buffer.append(m_socket->readAll());
    std::string data(m_buffer.constData(), static_cast<std::size_t>(m_buffer.size()));

    while (true) {
        Protocol::Packet packet;
        if (!Protocol::Packet::tryParse(data, packet)) {
            break;
        }
        processPacket(packet);
    }

    m_buffer = QByteArray(data.data(), static_cast<int>(data.size()));
}

void MainWindow::onSocketError()
{
    appendSystemLine(QString::fromUtf8("错误：") + m_socket->errorString());
    updateButtonStates();
}

void MainWindow::appendChatMessage(const QString &sender, const QString &message)
{
    const QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    const QString color = chatHeaderColor(sender);
    const QString html = QString(
                             "<table style='margin:0 0 10px 0;border:0;border-collapse:collapse;' cellspacing='0' cellpadding='0'>"
                             "<tr>"
                             "<td style='padding:0;'>"
                             "<span style='color:%4;font-weight:600;'>%1</span>"
                             "<span style='color:%4;'> %2</span>"
                             "</td>"
                             "</tr>"
                             "<tr>"
                             "<td style='padding:6px 0 0 2em;white-space:pre-wrap;color:#000000;'>%3</td>"
                             "</tr>"
                             "</table>")
                             .arg(sender.toHtmlEscaped(),
                                  time.toHtmlEscaped(),
                                  message.toHtmlEscaped(),
                                  color);

    QTextCursor cursor = ui->textBrowser_chat->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertHtml(html);
    cursor.insertHtml("<div style='margin:0;padding:0;'></div>");
    ui->textBrowser_chat->setTextCursor(cursor);
}

QString MainWindow::chatHeaderColor(const QString &sender) const
{
    if (!m_username.isEmpty() && sender == m_username) {
        return "#1f5eff";
    }
    return "#149c79";
}

void MainWindow::appendSystemLine(const QString &message)
{
    const QString time = QDateTime::currentDateTime().toString("hh:mm:ss");
    ui->systemInfoBrowser->append(QString("[%1] %2").arg(time, message));
}

void MainWindow::sendPacket(std::uint16_t cmd, const std::string &payload)
{
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        return;
    }

    const std::string data = Protocol::Packet(cmd, payload).serialize();
    m_socket->write(data.data(), static_cast<qint64>(data.size()));
}

void MainWindow::updateButtonStates()
{
    const bool connected = m_socket->state() == QAbstractSocket::ConnectedState;
    const bool loggedIn = connected && !m_username.isEmpty();

    ui->btn_connectServer->setEnabled(!connected);
    ui->btn_login->setEnabled(connected);
    ui->btn_register->setEnabled(connected);
    ui->btn_sendMsg->setEnabled(loggedIn);
    ui->btn_sendFile->setEnabled(loggedIn);
    ui->btn_downloadFile->setEnabled(loggedIn);
}

void MainWindow::processPacket(const Protocol::Packet &packet)
{
    switch (packet.command()) {
    case Protocol::CmdSystemMessage: {
        std::string text;
        if (Protocol::parseStringPayload(packet.payload(), text)) {
            appendSystemLine(fromStdString(text));
        }
        break;
    }
    case Protocol::CmdLoginOk: {
        std::string username;
        if (Protocol::parseStringPayload(packet.payload(), username)) {
            m_username = fromStdString(username);
            appendSystemLine(QString::fromUtf8("系统：当前登录用户：") + m_username);
            updateButtonStates();
        }
        break;
    }
    case Protocol::CmdChatMessage: {
        std::string sender;
        std::string message;
        if (Protocol::parseChatPayload(packet.payload(), sender, message)) {
            appendChatMessage(fromStdString(sender), fromStdString(message));
        }
        break;
    }
    case Protocol::CmdUserList: {
        std::vector<std::string> users;
        if (Protocol::parseStringListPayload(packet.payload(), users)) {
            ui->listWidget_users->clear();
            for (const std::string &user : users) {
                ui->listWidget_users->addItem(fromStdString(user));
            }
        }
        break;
    }
    case Protocol::CmdFileList: {
        std::vector<std::string> files;
        if (Protocol::parseStringListPayload(packet.payload(), files)) {
            ui->listWidget_files->clear();
            for (const std::string &file : files) {
                ui->listWidget_files->addItem(fromStdString(file));
            }
        }
        break;
    }
    case Protocol::CmdFileDownloadBegin: {
        std::string fileName;
        Protocol::QWord fileSize = 0;
        if (Protocol::parseFileBeginPayload(packet.payload(), fileName, fileSize)) {
            m_downloadFileName = fromStdString(fileName);
            m_downloadSize = fileSize;
            m_downloadReceived = 0;
        }
        break;
    }
    case Protocol::CmdFileDownloadChunk: {
        if (m_downloadFile.isOpen()) {
            m_downloadFile.write(packet.payload().data(), static_cast<qint64>(packet.payload().size()));
            m_downloadReceived += static_cast<quint64>(packet.payload().size());
        }
        break;
    }
    case Protocol::CmdFileDownloadEnd: {
        std::string fileName;
        Protocol::parseStringPayload(packet.payload(), fileName);
        if (m_downloadFile.isOpen()) {
            m_downloadFile.close();
        }

        if (m_downloadSize == 0 || m_downloadReceived == m_downloadSize) {
            appendSystemLine(QString::fromUtf8("系统：文件下载完成：") + fromStdString(fileName));
        } else {
            appendSystemLine(QString::fromUtf8("错误：文件下载不完整。"));
        }

        m_downloadFileName.clear();
        m_downloadSize = 0;
        m_downloadReceived = 0;
        break;
    }
    case Protocol::CmdError: {
        std::string text;
        if (Protocol::parseStringPayload(packet.payload(), text)) {
            if (m_downloadFile.isOpen()) {
                closeTransferFiles();
            }
            appendSystemLine(QString::fromUtf8("错误：") + fromStdString(text));
        }
        break;
    }
    default:
        appendSystemLine(QString::fromUtf8("系统：收到未知数据包。"));
        break;
    }
}

bool MainWindow::ensureLoggedIn()
{
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        appendSystemLine(QString::fromUtf8("提示：当前未连接服务器。"));
        return false;
    }

    if (m_username.isEmpty()) {
        appendSystemLine(QString::fromUtf8("提示：请先登录。"));
        return false;
    }

    return true;
}

void MainWindow::closeTransferFiles()
{
    const QString path = m_downloadFile.fileName();
    const bool incomplete = m_downloadFile.isOpen() && m_downloadSize > 0 && m_downloadReceived < m_downloadSize;
    if (m_downloadFile.isOpen()) {
        m_downloadFile.close();
    }
    if (incomplete && !path.isEmpty()) {
        QFile::remove(path);
    }
    m_downloadFileName.clear();
    m_downloadSize = 0;
    m_downloadReceived = 0;
}
