#include <QStandardItemModel>
#include <QInputDialog>
#include <QMessageBox>
#include <QFormLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QHostAddress>
#include <QDateTime>
#include "ui_window.h"
#include "clientwindow.h"
#include "constants.h"

ClientWindow::ClientWindow(QWidget* parent)
    : QWidget(parent), ui(new Ui::ClientWindow), clientCore(new ClientCore(this)),
      chatModel(new QStandardItemModel(this)), loadingScreen(new LoadingScreen)
{
    ui->setupUi(this);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    setMinimumSize(minWindowWidth, minWindowHeight);

    chatModel->insertColumn(0);
    ui->chatView->setModel(chatModel);
    ui->listWidget->setFocusPolicy(Qt::NoFocus);

    // connect for handle signals from logic view of client
    connect(clientCore, &ClientCore::connected, loadingScreen, &LoadingScreen::close);
    connect(clientCore, &ClientCore::connected, this, &ClientWindow::connected);
    connect(clientCore, &ClientCore::loggedIn, this, &ClientWindow::loggedIn);
    connect(clientCore, &ClientCore::loginError, this, &ClientWindow::loginError);
    connect(clientCore, &ClientCore::messageReceived, this, &ClientWindow::messageReceived);
    connect(clientCore, &ClientCore::disconnected, this, &ClientWindow::disconnected);
    connect(clientCore, &ClientCore::error, this, &ClientWindow::error);
    connect(clientCore, &ClientCore::userJoined, this, &ClientWindow::userJoined);
    connect(clientCore, &ClientCore::userLeft, this, &ClientWindow::userLeft);
    connect(clientCore, &ClientCore::informJoiner, this, &ClientWindow::informJoiner);
    // connect for send message
    connect(ui->sendButton, &QPushButton::clicked, this, &ClientWindow::sendMessage);
    connect(ui->messageEdit, &QLineEdit::returnPressed, this, &ClientWindow::sendMessage);

    attemptConnection();
}

ClientWindow::~ClientWindow()
{
    delete ui;
    delete clientCore;
    delete chatModel;
    delete loadingScreen;
}

QString ClientWindow::getTextDialog(const QString& title, const QString& label, const QString& defaultText = "")
{
    QDialog dialog(this);
    dialog.setWindowFlags(dialog.windowFlags() & ~Qt::WindowContextHelpButtonHint & ~Qt::WindowMaximizeButtonHint);
    dialog.setWindowTitle(title);
    dialog.resize(300, 100);

    QFormLayout form(&dialog);
    form.addRow(new QLabel(label));

    QLineEdit lineEdit(&dialog);
    lineEdit.setText(defaultText);
    form.addRow(&lineEdit);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);

    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(clientCore, &ClientCore::disconnected, &dialog, &QDialog::close);

    if (dialog.exec() == QDialog::Accepted) {
        return lineEdit.text();
    }
    return {};
}

void ClientWindow::attemptConnection()
{
    loadingScreen->show();
    clientCore->connectToServer(QHostAddress(HOST), PORT);
}

void ClientWindow::connected()
{
    const QString newUsername = getTextDialog("choose username", "username");
    if (newUsername.isEmpty() || newUsername.size() > maximumUserNameSize) {
        return clientCore->disconnectFromHost();
    }
    attemptLogin(newUsername);
}

void ClientWindow::attemptLogin(const QString& userName)
{
    clientCore->login(userName);
}

void ClientWindow::loggedIn()
{
    ui->sendButton->setEnabled(true);
    ui->messageEdit->setEnabled(true);
    ui->chatView->setEnabled(true);
    lastUserName.clear();
}

void ClientWindow::loginError(const QString& reason)
{
    QMessageBox::critical(this, tr("Error"), reason);
    connected();
}

QStringList ClientWindow::splitString(const QString& str, const int rowSize)
{
    QString temp = str;
    QStringList list;
    list.reserve(temp.size() / rowSize + 1);

    while (!temp.isEmpty()) {
        list.append(temp.left(rowSize).trimmed());
        temp.remove(0, rowSize);
    }
    return list;
}

QStringList ClientWindow::splitText(const QString& text)
{
    const QStringList words = text.split(QRegExp("[\r\n\t ]+"), QString::SkipEmptyParts);
    const int wordsCount    = words.size();
    QStringList rows;
    rows.append("");
    for (int i = 0, j = 0; i < wordsCount; ++i) {
        if (words[i].size() > maxMessageRowSize - rows[j].size()) {
            if (words[i].size() > maxMessageRowSize) {
                QStringList bigWords = splitString(words[i], maxMessageRowSize);
                for (const auto& bigWord : bigWords) {
                    if (rows[j].isEmpty()) {
                        rows[j] += bigWord + QString(" ");
                    }
                    rows.append(bigWord + QString(" "));
                    ++j;
                }
            } else {
                rows.append(words[i] + QString(" "));
                ++j;
            }
        } else {
            rows[j] += words[i] + QString(" ");
        }
    }
    return rows;
}

void ClientWindow::displayMessage(const QString& message, const int lastRowNumber, const int alignMask)
{
    QStringList rows    = splitText(message);
    const int rowsCount = rows.size();
    int currentRow      = lastRowNumber;
    const QString time  = QDateTime::currentDateTime().toString("hh:mm");
    for (int i = 0; i < rowsCount; ++i) {
        chatModel->insertRow(currentRow);
        if (i == rowsCount - 1) {
            chatModel->setData(chatModel->index(currentRow, 0), rows[i] + QString(" (") + time + QString(")"));
        } else {
            chatModel->setData(chatModel->index(currentRow, 0), rows[i]);
        }
        chatModel->setData(chatModel->index(currentRow, 0), int(alignMask | Qt::AlignVCenter), Qt::TextAlignmentRole);
        ui->chatView->scrollToBottom();
        ++currentRow;
    }
}

void ClientWindow::messageReceived(const QString& sender, const QString& message)
{
    int currentRow = chatModel->rowCount();
    if (lastUserName != sender) {
        lastUserName = sender;

        QFont boldFont;
        boldFont.setBold(true);

        chatModel->insertRow(currentRow);
        chatModel->setData(chatModel->index(currentRow, 0), sender);
        chatModel->setData(chatModel->index(currentRow, 0), int(Qt::AlignLeft | Qt::AlignVCenter),
                           Qt::TextAlignmentRole);
        chatModel->setData(chatModel->index(currentRow, 0), boldFont, Qt::FontRole);
        ++currentRow;
    }
    displayMessage(message, currentRow, Qt::AlignLeft);
}

void ClientWindow::sendMessage()
{
    const QString message = ui->messageEdit->text();
    if (message.isEmpty() || message.size() > maximumMessageSize) {
        return;
    }
    clientCore->sendMessage(message);

    int currentRow = chatModel->rowCount();
    displayMessage(message, currentRow, Qt::AlignRight);

    ui->messageEdit->clear();
    ui->chatView->scrollToBottom();
    lastUserName.clear();
}

void ClientWindow::disconnected()
{
    QMessageBox::warning(this, tr("Disconnected"), tr("The host terminated the connection"));

    ui->sendButton->setEnabled(false);
    ui->messageEdit->setEnabled(false);
    ui->chatView->setEnabled(false);
    lastUserName.clear();

    attemptConnection();
}

void ClientWindow::userEventImpl(const QString& username, const QString& event)
{
    const int newRow = chatModel->rowCount();
    chatModel->insertRow(newRow);
    chatModel->setData(chatModel->index(newRow, 0), tr("%1 %2").arg(username, event));
    chatModel->setData(chatModel->index(newRow, 0), Qt::AlignCenter, Qt::TextAlignmentRole);
    chatModel->setData(chatModel->index(newRow, 0), QBrush(Qt::gray), Qt::ForegroundRole);

    ui->chatView->scrollToBottom();
    lastUserName.clear();
}

void ClientWindow::userJoined(const QString& username)
{
    userEventImpl(username, "joined the group");
    ui->listWidget->addItem(username);
}

void ClientWindow::userLeft(const QString& username)
{
    userEventImpl(username, "left the group");
    QList<QListWidgetItem*> items = ui->listWidget->findItems(username, Qt::MatchExactly);
    if (items.isEmpty()) {
        return;
    }
    delete items.at(0);
}

void ClientWindow::informJoiner(const QStringList& usernames)
{
    for (const auto& username : usernames) {
        ui->listWidget->addItem(username);
    }
}

void ClientWindow::error(const QAbstractSocket::SocketError socketError)
{
    switch (socketError) {
        case QAbstractSocket::ConnectionRefusedError:
            break;
        case QAbstractSocket::RemoteHostClosedError:
            break;
        case QAbstractSocket::HostNotFoundError:
            break;
        case QAbstractSocket::SocketAccessError:
            break;
        case QAbstractSocket::SocketResourceError:
            break;
        case QAbstractSocket::SocketTimeoutError:
            QMessageBox::warning(this, tr("Error"), tr("Operation timed out"));
            return;
        case QAbstractSocket::DatagramTooLargeError:
            break;
        case QAbstractSocket::NetworkError:
            break;
        case QAbstractSocket::AddressInUseError:
            break;
        case QAbstractSocket::SocketAddressNotAvailableError:
            break;
        case QAbstractSocket::UnsupportedSocketOperationError:
            break;
        case QAbstractSocket::UnfinishedSocketOperationError:
            break;
        case QAbstractSocket::ProxyAuthenticationRequiredError:
            break;
        case QAbstractSocket::SslHandshakeFailedError:
            break;
        case QAbstractSocket::ProxyConnectionRefusedError:
            break;
        case QAbstractSocket::ProxyConnectionClosedError:
            break;
        case QAbstractSocket::ProxyConnectionTimeoutError:
            break;
        case QAbstractSocket::ProxyNotFoundError:
            break;
        case QAbstractSocket::ProxyProtocolError:
            break;
        case QAbstractSocket::OperationError:
            QMessageBox::warning(this, tr("Error"), tr("Operation failed, please try again"));
            return;
        case QAbstractSocket::SslInternalError:
            break;
        case QAbstractSocket::SslInvalidUserDataError:
            break;
        case QAbstractSocket::TemporaryError:
            break;
        default:
            Q_UNREACHABLE();
    }
    ui->sendButton->setEnabled(false);
    ui->messageEdit->setEnabled(false);
    ui->chatView->setEnabled(false);
    lastUserName.clear();
    attemptConnection();
}
