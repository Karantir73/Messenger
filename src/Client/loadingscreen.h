#ifndef LOADINGSCREEN_H
#define LOADINGSCREEN_H

#include <QWidget>
#include <QMovie>

QT_BEGIN_NAMESPACE
namespace Ui {
    class LoadingScreen;
}
QT_END_NAMESPACE

class LoadingScreen : public QWidget
{
    Q_OBJECT

public:
    explicit LoadingScreen(QWidget* parent = nullptr);
    ~LoadingScreen() override;

protected:
    void closeEvent(QCloseEvent* event) override;

signals:
    void closeSig();

private:
    Ui::LoadingScreen* ui;
    QMovie* movie;
};

#endif // LOADINGSCREEN_H
