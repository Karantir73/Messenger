#ifndef MESSENGER_MESSAGE_H
#define MESSENGER_MESSAGE_H

#include <QString>

class Message
{
public:
    Message() = default;
    Message(const QString& groupName, const QString& sender, const QString& message, const QString& time);
    [[nodiscard]] const QString& getGroupName() const;
    [[nodiscard]] const QString& getSender() const;
    [[nodiscard]] const QString& getMessage() const;
    [[nodiscard]] const QString& getTime() const;

private:
    QString groupName;
    QString sender;
    QString message;
    QString time;
};

#endif // MESSENGER_MESSAGE_H
