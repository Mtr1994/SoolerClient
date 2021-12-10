#ifndef TCPPACKCLIENT_H
#define TCPPACKCLIENT_H

#include "HPSocket/HPSocket.h"
#include "Listener/tcppackclientlistener.h"

#include <QObject>
#include <map>
#include <mutex>

/**
 * 每个 TCP 连接都是一个线程单独管理，所有的事件都发生在这个线程中
 * 唯一发生在主线程的事件为 OnClose
**/

typedef struct {
    CTcpPackClientPtr *client;
    TcpPackClientListener *listener;
} FileTransmissionChannel;

class TcpPackClient : public QObject
{
    Q_OBJECT
public:
    explicit TcpPackClient(QObject *parent = nullptr);
    ~TcpPackClient();

    void init();

    void connectServer(const QString &address, uint32_t port);

    void sendFile(const QString& file);

    void downloadFile(const QString& file);

    // 发送无参数命令
    void sendCommand(int cmd);

signals:
    void sgl_file_upload_process(const QString &filename, float process);
    void sgl_file_upload_check_finish(const QString &filename, bool status);

private:
    // 建立临时文件传输通道
    CTcpPackClientPtr *createFileTransmitChannel(const QString &extra);

    // 发送上传文件请求
    void sendFileUploadRequest(const QString &path, const QString &filename, uint64_t size);

    // 发送文件
    void sendUploadFile(int32_t uid, const QString &originpath, const QString &targetpath);

    // 发送登录
    void sendLogin(const QString &user, const QString &pwd);

    // 发送下载文件请求
    void sendFileDownloadRequest(const QString &filename, const QString &targetpath);

    // 下载文件
    void sendDownloadFile(int32_t uid, const QString &filename, const QString &targetpath, uint64_t size);

private slots:
    void slot_thread_socket_connect();

    void slot_thread_socket_close(ITcpClient *pSender);

    void slot_thread_file_upload_request_response(const ProtoBufMessage &message);

    void slot_thread_file_upload_response(const ProtoBufMessage &message);

    void slot_thread_file_upload_check_response(const ProtoBufMessage &message);

    void slot_thread_login(const ProtoBufMessage &message);

    void slot_thread_file_download_request_response(const ProtoBufMessage &message);

    void slot_thread_file_download_response(const ProtoBufMessage &message);

private:
    // 服务器信息
    QString mServerAddress;
    uint32_t mServerPort;

    // 短消息
    TcpPackClientListener mTcpPackClientListener;
    CTcpPackClientPtr mTcpPackClientPtr;
    uint32_t mClientStatus = SS_STOPPED;

    // 缓存上传传输进度 (uint64 前 4 字节表示总大小，后 4 字节表示已写入大小)
    std::map<std::string, uint64_t> mMapTransmitFileProcess;
    std::mutex mMutex;

    // 文件传输通道
    std::map<std::string, FileTransmissionChannel> mMapFileTransmissionChannel;
    std::mutex mMutexFileTransmission;
};

#endif // TCPPACKCLIENT_H
