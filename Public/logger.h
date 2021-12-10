#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <fstream>

#define LOGGER(...) Logger::getInstance()->debug( __VA_ARGS__, __FILE__, __LINE__, __FUNCTION__)

class QFile;
class Logger
{
public:
    static Logger* getInstance();

    bool init();

    void debug(const QString& msg, const std::string& file, int line, const std::string& func);

private:
    Logger();
    ~Logger();
    Logger(const Logger* logger) = delete;
    Logger& operator=(const Logger& logger) = delete;

    void printFile(const QString& msg, const std::string& file, int line, const std::string& func);
    void printConsole(const QString& msg, const std::string& file, int line, const std::string& func);

private:
    std::fstream mFileDescripter;
};

#endif // LOGGER_H
