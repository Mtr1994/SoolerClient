#ifndef TCPPACKCLIENTLISTENER_H
#define TCPPACKCLIENTLISTENER_H

#include "HPSocket/HPSocket.h"

#include <QObject>

class ProtoBufMessage;
class TcpPackClientListener : public QObject, public ITcpClientListener
{
    Q_OBJECT
public:
    explicit TcpPackClientListener(QObject *parent = nullptr);
    ~TcpPackClientListener();

    EnHandleResult OnPrepareConnect(ITcpClient* pSender, CONNID dwConnID, SOCKET socket) override;
    EnHandleResult OnConnect(ITcpClient* pSender, CONNID dwConnID) override;
    EnHandleResult OnHandShake(ITcpClient* pSender, CONNID dwConnID) override;
    EnHandleResult OnReceive(ITcpClient* pSender, CONNID dwConnID, int iLength) override;
    EnHandleResult OnReceive(ITcpClient* pSender, CONNID dwConnID, const BYTE* pData, int iLength) override;
    EnHandleResult OnSend(ITcpClient* pSender, CONNID dwConnID, const BYTE* pData, int iLength) override;
    EnHandleResult OnClose(ITcpClient* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode) override;

signals:
    // 套接字关闭
    void sgl_thread_socket_close(ITcpClient *pSender);

    // 套接字连接到服务器
    void sgl_thread_socket_connect();

    // 文件上传主动上报回复
    void sgl_thread_file_upload_request_response(const ProtoBufMessage &message);

    // 文件上传回复
    void sgl_thread_file_upload_response(const ProtoBufMessage &message);

    // 文件上传验证回复
    void sgl_thread_file_upload_check_response(const ProtoBufMessage &message);

    // 登录回复
    void sgl_thread_login(const ProtoBufMessage &message);

    // 文件下载请求回复
    void sgl_thread_file_download_request_response(const ProtoBufMessage &message);

    // 文件下载回复
    void sgl_thread_file_download_response(const ProtoBufMessage &message);

};

#endif // TCPPACKCLIENTLISTENER_H
