#include "tcppackclientlistener.h"
#include "ProtoBuf/sooler.pb.h"

// test
#include <QDebug>
#include <QThread>

TcpPackClientListener::TcpPackClientListener(QObject *parent) : QObject(parent)
{

}

TcpPackClientListener::~TcpPackClientListener()
{
    qDebug() << "delete listener";
}

EnHandleResult TcpPackClientListener::OnPrepareConnect(ITcpClient *pSender, CONNID dwConnID, SOCKET socket)
{
    Q_UNUSED(pSender); Q_UNUSED(dwConnID); Q_UNUSED(socket);
    return HR_OK;
}

EnHandleResult TcpPackClientListener::OnConnect(ITcpClient *pSender, CONNID dwConnID)
{
    //  子线程
    Q_UNUSED(pSender); Q_UNUSED(dwConnID);
    qDebug() << "connect" << QThread::currentThreadId();
    emit sgl_thread_socket_connect();
    return HR_OK;
}

EnHandleResult TcpPackClientListener::OnHandShake(ITcpClient *pSender, CONNID dwConnID)
{
    Q_UNUSED(pSender); Q_UNUSED(dwConnID);
    return HR_OK;
}

EnHandleResult TcpPackClientListener::OnReceive(ITcpClient *pSender, CONNID dwConnID, int iLength)
{
    Q_UNUSED(pSender); Q_UNUSED(dwConnID); Q_UNUSED(iLength);
    return HR_OK;
}

EnHandleResult TcpPackClientListener::OnReceive(ITcpClient *pSender, CONNID dwConnID, const BYTE *pData, int iLength)
{
    // listener 所在线程
    Q_UNUSED(pSender); Q_UNUSED(dwConnID); Q_UNUSED(iLength);
    QByteArray array = QByteArray::fromRawData((const char*)pData, iLength);
    ProtoBufMessage message;
    bool status = message.ParseFromArray(array.data(), array.length());

    if (!status)
    {
        // 日志记录出现解析失败的命令
        qDebug() << "parse error";
        return HR_OK;
    }

    // 赋值解析后的消息体
    switch (message.cmd())
    {
    case COMMAND::UNKNOW:
        break;
    case COMMAND::LOGINRESPONSE:
        emit sgl_thread_login(message);
        break;
    case COMMAND::FILEUPLOADREQUESTRESPONSE:
        emit sgl_thread_file_upload_request_response(message);
        break;
    case COMMAND::FILEUPLOADRESPONSE:
        emit sgl_thread_file_upload_response(message);
        break;
    case COMMAND::FILEUPLOADCHECKRESPONSE:
        emit sgl_thread_file_upload_check_response(message);
        break;
    case COMMAND::FILEDOWNLOADREQUESTRESPONSE:
        emit sgl_thread_file_download_request_response(message);
    case COMMAND::FILEDOWNLOADRESPONSE:
        emit sgl_thread_file_download_response(message);
        break;
    default:
        break;
    }
    return HR_OK;
}

EnHandleResult TcpPackClientListener::OnSend(ITcpClient *pSender, CONNID dwConnID, const BYTE *pData, int iLength)
{
    Q_UNUSED(pSender); Q_UNUSED(dwConnID); Q_UNUSED(pData);Q_UNUSED(iLength);
    return HR_OK;
}

EnHandleResult TcpPackClientListener::OnClose(ITcpClient *pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode)
{
    // 主线程触发
    Q_UNUSED(pSender); Q_UNUSED(dwConnID); Q_UNUSED(enOperation);Q_UNUSED(iErrorCode);
    qDebug() << "close" << QThread::currentThreadId();
    emit sgl_thread_socket_close(pSender);
    return HR_OK;
}

