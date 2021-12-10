#include "tcppackclient.h"
#include "Public/common.h"
#include "Public/threadpool.h"
#include "ProtoBuf/sooler.pb.h"
#include "Public/logger.h"
#include "Public/util.h"

#include <QFileInfo>
#include <fstream>
#include <chrono>
#include <qmetatype.h>
#include <QDir>

using namespace std;

// test
#include <QDebug>
#include <QThread>
#include <list>
#include <xstring>

TcpPackClient::TcpPackClient(QObject *parent) : QObject(parent)
{
    init();
}

TcpPackClient::~TcpPackClient()
{

}

void TcpPackClient::init()
{
    qRegisterMetaType<ProtoBufMessage>("ProtoBufMessage");

    mTcpPackClientPtr.Attach(TcpPackClient_Creator::Create(&mTcpPackClientListener));
    mTcpPackClientPtr->SetPackHeaderFlag(gl_server_head_flag);
    mTcpPackClientPtr->SetMaxPackSize(gl_server_max_pack_size);
    mTcpPackClientPtr->SetExtra(new QString("main channel"));

    connect(&mTcpPackClientListener, &TcpPackClientListener::sgl_thread_file_download_request_response, this, &TcpPackClient::slot_thread_file_download_request_response, Qt::QueuedConnection);
    connect(&mTcpPackClientListener, &TcpPackClientListener::sgl_thread_file_upload_request_response, this, &TcpPackClient::slot_thread_file_upload_request_response, Qt::QueuedConnection);
    connect(&mTcpPackClientListener, &TcpPackClientListener::sgl_thread_socket_connect, this, &TcpPackClient::slot_thread_socket_connect, Qt::QueuedConnection);
    connect(&mTcpPackClientListener, &TcpPackClientListener::sgl_thread_socket_close, this, &TcpPackClient::slot_thread_socket_close, Qt::QueuedConnection);
    connect(&mTcpPackClientListener, &TcpPackClientListener::sgl_thread_login, this, &TcpPackClient::slot_thread_login, Qt::QueuedConnection);
}

void TcpPackClient::connectServer(const QString &address, uint32_t port)
{
    mServerAddress = address;
    mServerPort = port;
    mTcpPackClientPtr->Start(LPCTSTR(address.toStdString().data()), port);

    LOGGER(QString("服务器已连接：<IP: %1，PORT: %2>").arg(address, QString::number(port)));
}

void TcpPackClient::sendFile(const QString &file)
{
    if (mTcpPackClientPtr->GetState() != SS_STARTED)
    {
        // 发送消息给客户端
        qDebug() << "客户端断开";
        return;
    }

    QFileInfo info(file);
    if (!info.exists())
    {
        // 发送消息给客户端
        qDebug() << "文件不存在";
        return;
    }

    auto func = std::bind(&TcpPackClient::sendFileUploadRequest, this, file, info.fileName(), info.size());
    ThreadPool::getInstance()->enqueue(func);
}

void TcpPackClient::downloadFile(const QString &file)
{
    if (mTcpPackClientPtr->GetState() != SS_STARTED)
    {
        // 发送消息给客户端
        qDebug() << "客户端断开";
        return;
    }

    auto func = std::bind(&TcpPackClient::sendFileDownloadRequest, this, file, "Video/Test");
    ThreadPool::getInstance()->enqueue(func);
}

void TcpPackClient::sendCommand(int cmd)
{
    switch (cmd) {
    case COMMAND::LOGIN:
        sendLogin("Mtr", "123456");
        break;
    default:
        break;
    }
}

CTcpPackClientPtr *TcpPackClient::createFileTransmitChannel(const QString &extra)
{
    CTcpPackClientPtr *channel = new CTcpPackClientPtr;
    TcpPackClientListener *listener = new TcpPackClientListener;
    (*channel).Attach(TcpPackClient_Creator::Create(listener));
    (*channel)->SetExtra(new QString(extra));

    connect(listener, &TcpPackClientListener::sgl_thread_file_upload_response, this, &TcpPackClient::slot_thread_file_upload_response, Qt::QueuedConnection);
    connect(listener, &TcpPackClientListener::sgl_thread_file_upload_check_response, this, &TcpPackClient::slot_thread_file_upload_check_response, Qt::QueuedConnection);
    connect(listener, &TcpPackClientListener::sgl_thread_file_download_response, this, &TcpPackClient::slot_thread_file_download_response, Qt::QueuedConnection);
    connect(listener, &TcpPackClientListener::sgl_thread_socket_connect, this, &TcpPackClient::slot_thread_socket_connect, Qt::QueuedConnection);
    connect(listener, &TcpPackClientListener::sgl_thread_socket_close, this, &TcpPackClient::slot_thread_socket_close, Qt::QueuedConnection);

    (*channel)->SetPackHeaderFlag(gl_server_head_flag);
    (*channel)->SetMaxPackSize(gl_server_max_pack_size);
    (*channel)->Start(LPCTSTR(mServerAddress.toStdString().data()), mServerPort);

    uint16_t tryCount = 0;
    while ((*channel)->GetState() != SS_STARTED)
    {
        tryCount++;
        if (tryCount == 10)
        {
            qDebug() << "无法连接至服务器";
            listener->blockSignals(true);
            (*channel)->Stop();
            delete (QString*)(*channel)->GetExtra();
            (*channel).Detach();
            delete listener;
            delete channel;
            return nullptr;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::unique_lock<std::mutex> lockFileTransmission(mMutexFileTransmission);
    FileTransmissionChannel newChannel = {channel, listener};
    mMapFileTransmissionChannel.insert(std::make_pair(extra.toStdString(), newChannel));
    lockFileTransmission.unlock();
    return channel;
}

void TcpPackClient::slot_thread_socket_connect()
{
    qDebug() << "客户端连上了" << QThread::currentThreadId();;
    mClientStatus = SS_STARTED;
}

void TcpPackClient::slot_thread_socket_close(ITcpClient *pSender)
{
    qDebug() << "客户端断开了" << QThread::currentThreadId();
    if (nullptr == pSender) return;
    QString extra = *(QString *)pSender->GetExtra();

    if (extra == "main channel")
    {
        mClientStatus = SS_STOPPED;
        delete (QString *)pSender->GetExtra();
    }
    else
    {
        std::lock_guard<std::mutex> lockFileTransmission(mMutexFileTransmission);
        auto iterChannel = mMapFileTransmissionChannel.find(extra.toStdString());
        if (iterChannel == mMapFileTransmissionChannel.end())
        {
             qDebug() << "传输通道检索异常";
        }
        else
        {
            delete (QString*)(*iterChannel->second.client)->GetExtra();
            delete iterChannel->second.listener;
            iterChannel->second.client->Detach();
            delete iterChannel->second.client;
            mMapFileTransmissionChannel.erase(iterChannel);
        }
    }
}

// 发送消息

void TcpPackClient::sendFileUploadRequest(const QString &path, const QString &filename, uint64_t size)
{
    // 线程发送数据
    FileUploadRequest *request = new FileUploadRequest;
    request->set_uid(9527);
    request->set_filesize(size);
    request->set_filename(filename.toStdString());
    request->set_originpath(path.toStdString());
    request->set_targetpath("Video/Test");

    ProtoBufMessage message;
    message.set_cmd(COMMAND::FILEUPLOADREQUEST);
    message.set_allocated_fileuploadrequest(request);

    QByteArray dataArray = QByteArray::fromStdString(message.SerializeAsString());

    mTcpPackClientPtr->Send((BYTE *)dataArray.data(), dataArray.length());
}

void TcpPackClient::sendUploadFile(int32_t uid, const QString &originpath, const QString &targetpath)
{
    fstream fileDescriptor(originpath.toStdString().data(), ios::in | ios::binary);
    if (!fileDescriptor.is_open())
    {
        // 发送文件打开失败消息
        qDebug() << "文件打开失败";
        return;
    }

    QFileInfo fileInfo(originpath);

    // 准备一个专属的文件传输通道
    CTcpPackClientPtr *channel = createFileTransmitChannel(QString("%1/%2/%3").arg(QString::number(uid), targetpath.data(), fileInfo.fileName()));

    uint32_t fileSize = fileInfo.size();
    uint32_t count = fileSize / gl_mss + ((fileSize % gl_mss) > 0 ? 1 : 0);

    // 缓存文件上传进度
    uint64_t total = fileSize;
    std::unique_lock<std::mutex> lock(mMutex);
    mMapTransmitFileProcess.insert(std::make_pair(fileInfo.fileName().toStdString(), total << 32));
    lock.unlock();

    char *data = new char[gl_mss];

    for (uint32_t i = 0; i < count; i++)
    {
        FileUpload *fileUpload = new FileUpload;
        fileUpload->set_uid(uid);
        fileUpload->set_filename(fileInfo.fileName().toStdString());
        fileUpload->set_index(i * gl_mss);
        fileUpload->set_targetpath(targetpath.toStdString().data());
        fileUpload->set_originpath(originpath.toStdString().data());

        uint32_t length = 0;
        if (count == 1)
        {
            length = fileSize;
        }
        else if (i == (count - 1))
        {
            length = fileSize - i * gl_mss;
        }
        else
        {
            length = gl_mss;
        }

        // qDebug() << "read size " << length;
        fileUpload->set_size(length);

        // 发送出现错误的时候，文件流需要回退
        fileDescriptor.read(data, length);

        QByteArray fileArray;
        fileArray.resize(length);
        memcpy(fileArray.data(), data, length);

       // qDebug() << "fileArray " << fileArray.toHex();

        fileUpload->set_data(fileArray.toStdString());

        ProtoBufMessage protobufmessage;
        protobufmessage.set_cmd(COMMAND::FILEUPLOAD);
        // response 会自动析构 response 对象
        protobufmessage.set_allocated_fileupload(fileUpload);

        QByteArray dataArray = QByteArray::fromStdString(protobufmessage.SerializeAsString());

      //  qDebug() << "sent " << i << "    " << dataArray.length();
      //  qDebug() << "array " << dataArray.toHex();

        if ((*channel)->GetState() != SS_STARTED)
        {
            qDebug() << "error happen";
            break;
        }

        (*channel)->Send((BYTE*)dataArray.data(), dataArray.length());
    }

    delete [] data;
}

void TcpPackClient::sendLogin(const QString &user, const QString &pwd)
{
    // 线程发送数据
    Login *login = new Login;
    login->set_username(user.toStdString());
    login->set_pwd(pwd.toStdString());
    login->set_datetime(0);

    ProtoBufMessage message;
    message.set_cmd(COMMAND::LOGIN);
    message.set_allocated_login(login);

    QByteArray dataArray = QByteArray::fromStdString(message.SerializeAsString());
    mTcpPackClientPtr->Send((BYTE *)dataArray.data(), dataArray.length());
}

void TcpPackClient::sendFileDownloadRequest(const QString &filename, const QString &targetpath)
{
    FileDownloadRequest *request = new FileDownloadRequest;
    request->set_uid(9527);
    request->set_filename(filename.toStdString());
    request->set_targetpath(targetpath.toStdString());

    ProtoBufMessage message;
    message.set_cmd(COMMAND::FILEDOWNLOADREQUEST);
    message.set_allocated_filedownloadrequest(request);

    QByteArray dataArray = QByteArray::fromStdString(message.SerializeAsString());
    mTcpPackClientPtr->Send((BYTE *)dataArray.data(), dataArray.length());
}

void TcpPackClient::sendDownloadFile(int32_t uid, const QString &filename, const QString &targetpath, uint64_t size)
{
    // 下载前创建缓存路径及文件
    QDir userDir(gl_file_download_root_path.data());
    if (!userDir.exists())
    {
        QDir rootDir;
        if(!rootDir.mkpath(gl_file_download_root_path.data()))
        {
            qDebug() << "创建文件夹失败";
            LOGGER(QString("创建用户文件夹失败 %1").arg(rootDir.absolutePath()));
            return;
        }
    }

    QString filePath = QString("%1/%2").arg(gl_file_download_root_path.data(), filename.data());
    std::fstream fileStream(filePath.toStdString(), std::ios::out | std::ios::binary | std::ios::trunc);
    if (!fileStream.is_open())
    {
        qDebug() << "创建文件失败";
        LOGGER(QString("创建目标文件失败 %1").arg(filePath));
        return;
    }
    else
    {
        fileStream.seekp(size - 1);
        fileStream.write(" ", 1);
        fileStream.close();
    }

    // 缓存文件下载进度
    uint64_t total = size;
    std::unique_lock<std::mutex> lock(mMutex);
    mMapTransmitFileProcess.insert(std::make_pair(filePath.toStdString(), total << 32));
    lock.unlock();

    // 建立文件传输通道

    CTcpPackClientPtr *channel = createFileTransmitChannel(filePath);

    // 线程发送数据
    FileDownload *request = new FileDownload;
    request->set_uid(uid);
    request->set_filename(filename.toStdString());
    request->set_targetpath(targetpath.toStdString());

    ProtoBufMessage message;
    message.set_cmd(COMMAND::FILEDOWNLOAD);
    message.set_allocated_filedownload(request);

    QByteArray dataArray = QByteArray::fromStdString(message.SerializeAsString());
    (*channel)->Send((BYTE *)dataArray.data(), dataArray.length());
}

// 发送消息

// 接收消息函数

void TcpPackClient::slot_thread_file_upload_request_response(const ProtoBufMessage &message)
{
    int32_t uid = message.fileuploadrequestresponse().uid();
    int32_t code = message.fileuploadrequestresponse().code();
    string description = message.fileuploadrequestresponse().description();
    string originpath = message.fileuploadrequestresponse().originpath();
    string targetpath = message.fileuploadrequestresponse().targetpath();

    if (code != STATUSCODE::SUCCESS)
    {
        qDebug() << "文件上报错误: " << description.data();
        return;
    }

    qDebug() << targetpath.data();

    // 发送文件数据
    auto func = std::bind(&TcpPackClient::sendUploadFile, this, uid, QString::fromStdString(originpath), QString::fromStdString(targetpath));
    ThreadPool::getInstance()->enqueue(func);
}

void TcpPackClient::slot_thread_file_upload_response(const ProtoBufMessage &message)
{
    int32_t uid = message.fileuploadresponse().uid();
    int32_t code = message.fileuploadresponse().code();
    int32_t size = message.fileuploadresponse().size();
    string description = message.fileuploadresponse().description();
    string filename = message.fileuploadresponse().filename();
    string targetpath = message.fileuploadresponse().targetpath();
    string originpath = message.fileuploadresponse().originpath();

    if (code != STATUSCODE::SUCCESS)
    {
        qDebug() << "文件上传错误: " << description.data();
        return;
    }

    std::unique_lock<std::mutex> lock(mMutex);
    auto iterFile = mMapTransmitFileProcess.find(filename);
    if (iterFile == mMapTransmitFileProcess.end())
    {
        qDebug() << "找不到本地缓存: ";
        return;
    }

    uint64_t total = iterFile->second >> 32;
    uint64_t uploadSize = iterFile->second & 0xffffffff;
    uploadSize += size;
    iterFile->second = (total << 32) + uploadSize;

    emit sgl_file_upload_process(filename.data(), uploadSize * 100.00 / total);
    if (uploadSize == total)
    {
        QString fileFlag = QString("%1/%2/%3").arg(QString::number(uid), targetpath.data(), filename.data());
        auto iterChannel = mMapFileTransmissionChannel.find(fileFlag.toStdString());
        if (iterChannel == mMapFileTransmissionChannel.end())
        {
             qDebug() << "传输通道检索异常";
        }
        else
        {
            QString filePath = QString("%1").arg(originpath.data());
            qDebug() << "path " << filePath;
            QByteArray md5 = Util::getFileMd5(filePath);

            qDebug() << "md5 " << md5.toHex();

            // 文件发送完毕，请求核验
            FileUploadCheck *fileUploadCheck = new FileUploadCheck;
            fileUploadCheck->set_uid(uid);
            fileUploadCheck->set_targetpath(targetpath);
            fileUploadCheck->set_filename(filename);
            fileUploadCheck->set_md5(md5);

            ProtoBufMessage message;
            message.set_cmd(COMMAND::FILEUPLOADCHECK);
            message.set_allocated_fileuploadcheck(fileUploadCheck);
            QByteArray dataArray = QByteArray::fromStdString(message.SerializeAsString());
            (*(iterChannel->second.client))->Send((BYTE *)dataArray.data(), dataArray.length());
        }
    }
    else if (uploadSize > total)
    {
        qDebug() << "文件大小异常";
    }
}

void TcpPackClient::slot_thread_file_upload_check_response(const ProtoBufMessage &message)
{
    int32_t uid = message.fileuploadcheckresponse().uid();
    int32_t code = message.fileuploadcheckresponse().code();
    string description = message.fileuploadcheckresponse().description();
    string filename = message.fileuploadcheckresponse().filename();
    string targetpath = message.fileuploadcheckresponse().targetpath();

    if (code != STATUSCODE::SUCCESS)
    {
        qDebug() << "文件校验异常: " << description.data();
        return;
    }

    qDebug() << "filename " << filename.data();

    std::unique_lock<std::mutex> lock(mMutex);
    auto iterFile = mMapTransmitFileProcess.find(filename);
    if (iterFile == mMapTransmitFileProcess.end())
    {
        qDebug() << "找不到本地缓存: ";
        return;
    }

    emit sgl_file_upload_check_finish(filename.data(), code == STATUSCODE::SUCCESS);

    mMapTransmitFileProcess.erase(iterFile);
    lock.unlock();

    QString fileFlag = QString("%1/%2/%3").arg(QString::number(uid), targetpath.data(), filename.data());
    auto iterChannel = mMapFileTransmissionChannel.find(fileFlag.toStdString());
    if (iterChannel == mMapFileTransmissionChannel.end())
    {
         qDebug() << "传输通道检索异常";
    }
    else
    {
        qDebug() << "检验完成" << code << " " << description.data();
        (*(iterChannel->second.client))->Stop();
    }
}

void TcpPackClient::slot_thread_login(const ProtoBufMessage &message)
{
    string description = message.loginresponse().description();
    qDebug() << "收到登录回复";
    qDebug() << QString::fromStdString(description).toLocal8Bit().toStdString().data();
}

void TcpPackClient::slot_thread_file_download_request_response(const ProtoBufMessage &message)
{
    int32_t uid = message.filedownloadrequestresponse().uid();
    int32_t code = message.filedownloadrequestresponse().code();
    int32_t size = message.filedownloadrequestresponse().size();
    string description = message.filedownloadrequestresponse().description();
    string filename = message.filedownloadrequestresponse().filename();
    string targetpath = message.filedownloadrequestresponse().targetpath();

    if (code != STATUSCODE::SUCCESS)
    {
        qDebug() << "文件下载错误: " << description.data();
        LOGGER(QString("文件下载请求错误 %1").arg(filename.data()));
        return;
    }

    // 准备下载文件
    auto func = std::bind(&TcpPackClient::sendDownloadFile, this, uid, QString::fromStdString(filename), QString::fromStdString(targetpath), size);
    ThreadPool::getInstance()->enqueue(func);
}

void TcpPackClient::slot_thread_file_download_response(const ProtoBufMessage &message)
{
    // 线程中执行文件写入
    auto func = [message, this](const ProtoBufMessage &)
    {
        auto fileDownload = message.filedownloadresponse();
        QString filename = QString::fromStdString(fileDownload.filename());
        if (fileDownload.code() != STATUSCODE::SUCCESS)
        {
            LOGGER(QString("文件下载错误 %1 %2").arg(filename, fileDownload.description().data()));
            return;
        }

        QString filePath = QString("%1/%2").arg(gl_file_download_root_path.data(), filename);
        fstream fileStream(filePath.toStdString(), ios::out | ios::binary | ios::in);
        if (!fileStream.is_open())
        {
            qDebug() << "打开文件失败" << fileDownload.index() << "  " << filePath;
            LOGGER(QString("打开目标文件失败 %1").arg(filePath));
        }
        else
        {
            fileStream.seekp(fileDownload.index(), ios::beg);
            fileStream.write(fileDownload.data().data(), fileDownload.size());

            std::unique_lock<std::mutex> lock(mMutex);
            auto iterFile = mMapTransmitFileProcess.find(filePath.toStdString());
            if (iterFile == mMapTransmitFileProcess.end())
            {
                qDebug() << "找不到本地进度缓存: ";
                return;
            }

            uint64_t total = iterFile->second >> 32;
            uint64_t downloadSize = iterFile->second & 0xffffffff;
            downloadSize += fileDownload.size();
            iterFile->second = (total << 32) + downloadSize;

            if (downloadSize == total)
            {
                mMapTransmitFileProcess.erase(iterFile);
                qDebug() << "文件下载完成 " << filePath;

                QString fileFlag = QString("%1/%2").arg(gl_file_download_root_path.data(), filename.data());
                auto iterChannel = mMapFileTransmissionChannel.find(fileFlag.toStdString());
                if (iterChannel == mMapFileTransmissionChannel.end())
                {
                     qDebug() << "传输通道检索异常";
                }
                else
                {
                    (*(iterChannel->second.client))->Stop();
                }
            }
        }
    };

    ThreadPool::getInstance()->enqueue(func, message);
}

// 接收消息函数
