#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QDateTime>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QLineEdit>
#include <QListWidgetItem>
#include <QProcessEnvironment>
#include <QTextCursor>
#include <QTcpSocket>

namespace {

constexpr qint64 kFileChunkSize = 4096;
constexpr quint16 kDefaultServerPort = 8888;
const char *kDefaultServerHost = "123.207.62.1";
const char *kServerHostEnv = "IM_CHAT_SERVER_HOST";
const char *kServerPortEnv = "IM_CHAT_SERVER_PORT";

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
    ui->lineEditIp->setText(serverHost());
    ui->spinBoxPort->setValue(static_cast<int>(serverPort()));
    appendSystemLine(QStringLiteral("Client ready. Connect to a server first."));
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
        appendSystemLine(QStringLiteral("Message is empty."));
        return;
    }

    sendPacket(Protocol::CmdChatMessage, Protocol::makeStringPayload(toStdString(content)));
    ui->textEdit_input->clear();
}

void MainWindow::on_btn_login_clicked()
{
    const QString username = ui->lineEdit_username->text().trimmed();
    if (username.isEmpty()) {
        appendSystemLine(QStringLiteral("Username is required."));
        return;
    }

    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        appendSystemLine(QStringLiteral("Connect to the server before login."));
        return;
    }

    sendPacket(Protocol::CmdLogin, Protocol::makeStringPayload(toStdString(username)));
}

void MainWindow::on_btn_register_clicked()
{
    on_btn_login_clicked();
}

void MainWindow::on_btn_sendFile_clicked()
{
    if (!ensureLoggedIn()) {
        return;
    }

    const QString filePath = QFileDialog::getOpenFileName(this, QStringLiteral("Select file"));
    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        appendSystemLine(QStringLiteral("Failed to open the file."));
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
            appendSystemLine(QStringLiteral("Failed to read the file."));
            file.close();
            return;
        }

        sendPacket(Protocol::CmdFileUploadChunk,
                   std::string(chunk.constData(), static_cast<std::size_t>(chunk.size())));
    }

    file.close();
    sendPacket(Protocol::CmdFileUploadEnd, std::string());
    appendSystemLine(QStringLiteral("Upload completed: ") + fileName);
}

void MainWindow::on_btn_downloadFile_clicked()
{
    if (!ensureLoggedIn()) {
        return;
    }

    QListWidgetItem *item = ui->listWidget_files->currentItem();
    if (item == nullptr) {
        appendSystemLine(QStringLiteral("Select a file to download."));
        return;
    }

    const QString fileName = item->text();
    const QString savePath = QFileDialog::getSaveFileName(this, QStringLiteral("Save file"), fileName);
    if (savePath.isEmpty()) {
        return;
    }

    closeTransferFiles();
    m_downloadFile.setFileName(savePath);
    if (!m_downloadFile.open(QIODevice::WriteOnly)) {
        appendSystemLine(QStringLiteral("Failed to create the local file."));
        return;
    }

    m_downloadFileName = fileName;
    m_downloadSize = 0;
    m_downloadReceived = 0;
    sendPacket(Protocol::CmdFileDownloadRequest, Protocol::makeStringPayload(toStdString(fileName)));
    appendSystemLine(QStringLiteral("Downloading: ") + fileName);
}

void MainWindow::on_btn_connectServer_clicked()
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        appendSystemLine(QStringLiteral("Already connected."));
        return;
    }

    const QString host = serverHost();
    const quint16 port = serverPort();
    appendSystemLine(QString("Connecting to %1:%2 ...").arg(host).arg(port));
    m_socket->connectToHost(host, port);
    updateButtonStates();
}

void MainWindow::onConnected()
{
    appendSystemLine(QStringLiteral("Connected to server."));
    updateButtonStates();
}

void MainWindow::onDisconnected()
{
    appendSystemLine(QStringLiteral("Disconnected from server."));
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
    appendSystemLine(QStringLiteral("Socket error: ") + m_socket->errorString());
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
            appendSystemLine(QStringLiteral("Logged in as ") + m_username);
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
            appendSystemLine(QStringLiteral("Download completed: ") + fromStdString(fileName));
        } else {
            appendSystemLine(QStringLiteral("Download incomplete."));
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
            appendSystemLine(QStringLiteral("Server error: ") + fromStdString(text));
        }
        break;
    }
    default:
        appendSystemLine(QStringLiteral("Unknown packet received."));
        break;
    }
}

bool MainWindow::ensureLoggedIn()
{
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        appendSystemLine(QStringLiteral("Not connected to server."));
        return false;
    }

    if (m_username.isEmpty()) {
        appendSystemLine(QStringLiteral("Please login first."));
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

QString MainWindow::serverHost() const
{
    const QString uiHost = ui->lineEditIp->text().trimmed();
    if (!uiHost.isEmpty()) {
        return uiHost;
    }

    const QString envHost = QProcessEnvironment::systemEnvironment().value(kServerHostEnv).trimmed();
    return envHost.isEmpty() ? QString::fromLatin1(kDefaultServerHost) : envHost;
}

quint16 MainWindow::serverPort() const
{
    const int uiPort = ui->spinBoxPort->value();
    if (uiPort > 0 && uiPort <= 65535) {
        return static_cast<quint16>(uiPort);
    }

    bool ok = false;
    const QString portText = QProcessEnvironment::systemEnvironment().value(kServerPortEnv).trimmed();
    const int port = portText.toInt(&ok);
    if (!ok || port <= 0 || port > 65535) {
        if (!portText.isEmpty()) {
            qWarning() << "Invalid" << kServerPortEnv << "value:" << portText;
        }
        return kDefaultServerPort;
    }

    return static_cast<quint16>(port);
}
