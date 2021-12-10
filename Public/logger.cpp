#include "logger.h"

#include <QFile>
#include <QDebug>
#include <string>
#include <QDateTime>

using namespace std;

Logger *Logger::getInstance()
{
    static Logger logger;
    return &logger;
}

bool Logger::init()
{
    mFileDescripter = fstream("logs.txt", std::ios::out);
    return mFileDescripter.is_open();
}

void Logger::debug(const QString &msg, const string& file, int line, const string& func)
{
    if (mFileDescripter.is_open()) printFile(msg, file, line, func);
    else printConsole(msg, file, line, func);
}

Logger::Logger()
{

}

Logger::~Logger()
{

}

void Logger::printFile(const QString& msg, const std::string& file, int line, const std::string& func)
{
    QString cleanfile = QString(file.data()).split('\\').last().toLower();
    QString cleanfunc = QString(func.data()).split("::").last().toLower();
    QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    QString str = QString("[%1%2%3%4] : %5\r").arg(time, 23).arg(cleanfile.data(), 32).arg(QString::number(line), 5).arg(cleanfunc.data(), 32).arg(msg);

    mFileDescripter.write(str.toLower().toStdString().data(), str.toStdString().length());
    mFileDescripter.flush();
}

void Logger::printConsole(const QString& msg, const std::string& file, int line, const std::string& func)
{
    QString cleanfile = QString(file.data()).split('\\').last();
    QString cleanfunc = QString(func.data()).split("::").last();
    QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    QString str = QString("%1 %2 %3 %4:    %5 \r").arg(time, 23).arg(cleanfile.data(), 32).arg(line, 4).arg(cleanfunc.data(), 32).arg(msg);
}
