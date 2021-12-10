#ifndef UTIL_H
#define UTIL_H

#include <QByteArray>

class Util
{
public:
    Util();

    static QByteArray getFileMd5(const QString &filePath);
};

#endif // UTIL_H
