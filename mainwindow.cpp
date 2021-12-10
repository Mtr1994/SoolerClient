#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QScreen>
#include <QDesktopWidget>

// test

#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    init();

    setWindowTitle("木头人可视化模板");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::init()
{
    float width = 1000;
    float height = 320;

    resize(width, height);

    ui->progressBar->setValue(0);

    connect(ui->btnSendFile, &QPushButton::clicked, this, &MainWindow::slot_file_send);
    connect(ui->btnSendCommand, &QPushButton::clicked, this, &MainWindow::slot_command_send);

    connect(ui->btnDownloadFile, &QPushButton::clicked, this, &MainWindow::slot_download);

    connect(&mTcpPackClient, &TcpPackClient::sgl_file_upload_process, this, &MainWindow::slot_file_upload_process);

    mTcpPackClient.connectServer("10.1.51.234", 57131);
   // mTcpPackClient.connectServer("10.10.37.186", 57131);
}

void MainWindow::slot_file_send()
{
   // mTcpPackClient.sendFile("E:\\document\\Video\\demo3.txt");
    mTcpPackClient.sendFile("E:\\document\\Video\\demo4.mp4");
  //  mTcpPackClient.sendFile("E:\\document\\Video\\demo4.mp4");
}

void MainWindow::slot_command_send()
{
    qDebug() << "Sent Command";

    login();
}

void MainWindow::slot_file_upload_process(const QString &filename, float process)
{
    if (filename == "demo7.mp4" || filename == "demo4.mp4")
    {
        ui->progressBar->setValue(process);
        ui->progressBar->setFormat(QString("%1 %").arg(QString::number(process, 'f', 2)));

        if (process == 100)
        {
            qDebug() << "文件上传完成 " << filename;
        }
    }
}

void MainWindow::login()
{
    mTcpPackClient.sendCommand(1);
}

void MainWindow::slot_download()
{
    mTcpPackClient.downloadFile("demo7.mp4");
}

