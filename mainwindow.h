#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "Client/tcppackclient.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void init();

private slots:
    void slot_file_send();

    void slot_command_send();

    void slot_file_upload_process(const QString &filename, float process);

    void login();

    void slot_download();

private:
    Ui::MainWindow *ui;

    TcpPackClient mTcpPackClient;
};
#endif // MAINWINDOW_H
