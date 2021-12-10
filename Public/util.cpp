#include "util.h"

#include <QFileInfo>
#include <QCryptographicHash>
#include <fstream>

using namespace std;

Util::Util()
{

}

QByteArray Util::getFileMd5(const QString &filePath)
{
    fstream  fileDescripter(filePath.toStdString(), ios::in | ios::binary);
    if (!fileDescripter.is_open())
    {
        return QByteArray();
    }

    QFileInfo info(filePath);
    QCryptographicHash hashTool(QCryptographicHash::Md5);

    int64_t bytesToWrite = info.size();
    int64_t loadSize = 1024 * 1024 * 2;
    QByteArray buf;
    while (true)
    {
        if(bytesToWrite <= 0) break;

        int64_t len = qMin(bytesToWrite, loadSize);
        buf.resize(len);
        fileDescripter.read(buf.data(), len);
        hashTool.addData(buf);
        bytesToWrite -= buf.length();
    }

    return hashTool.result();
}
