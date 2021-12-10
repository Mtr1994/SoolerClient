#include "mainwindow.h"
#include "Public/logger.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 加载样式
    qApp->setStyleSheet("file:///:/Resourse/qss/style.qss");

    // 初始化日志
    Logger::getInstance()->init();

    MainWindow w;
    w.show();

    return a.exec();
}
