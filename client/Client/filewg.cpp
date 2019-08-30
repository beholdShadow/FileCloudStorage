#include "filewg.h"
#include "ui_filewg.h"
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QMessageBox>

filewg::filewg(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::filewg)
{
    ui->setupUi(this);

    initListWidget();

    addActionMenu();

    m_manager=m_cm.getNetManager();

    checkTaskList();

}

filewg::~filewg()
{
    delete ui;
}

// 初始化listWidget文件列表
void filewg::initListWidget()
{
    ui->listWidget->setViewMode(QListView::IconMode);
    ui->listWidget->setIconSize(QSize(80,80));
    ui->listWidget->setGridSize(QSize(100,120));

    ui->listWidget->setResizeMode(QListView::Adjust);

    ui->listWidget->setMovement(QListView::Static);

    ui->listWidget->setSpacing(30);

    ui->listWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->listWidget,&QListWidget::customContextMenuRequested,this,&filewg::rightMenu);

    connect(ui->listWidget,&QListWidget::itemPressed,[=](QListWidgetItem *item)
            {
                if(item->text()=="上传文件")
                    addUploadFiles();
            });
}

// 添加右键菜单
void filewg::addActionMenu()
{
//    MyMenu   *m_menu;           // 菜单1
//    QAction *m_downloadAction; // 下载
//    QAction *m_shareAction;    // 分享
//    QAction *m_delAction;      // 删除
//    QAction *m_propertyAction; // 属性

//    MyMenu   *m_menuEmpty;          // 菜单2
//    QAction *m_pvAscendingAction;  // 按下载量升序
//    QAction *m_pvDescendingAction; // 按下载量降序
//    QAction *m_refreshAction;      // 刷新
//    QAction *m_uploadAction;       // 上传

    m_menu=new MyMenu(this);
    m_downloadAction=new QAction("下载",this);
    m_delAction=new QAction("删除",this);
    m_shareAction=new QAction("分享",this);
    m_propertyAction=new QAction("属性",this);
    m_menu->addAction(m_downloadAction);
    m_menu->addAction(m_delAction);
    m_menu->addAction(m_shareAction);
    m_menu->addAction(m_propertyAction);

    m_menuEmpty=new MyMenu(this);
    m_pvAscendingAction=new QAction("按下载量升序",this);
    m_pvDescendingAction=new QAction("按下载量降序",this);
    m_refreshAction=new QAction("刷新",this);
    m_uploadAction=new QAction("上传",this);
    m_menuEmpty->addAction(m_pvAscendingAction);
    m_menuEmpty->addAction(m_pvDescendingAction);
    m_menuEmpty->addAction(m_refreshAction);
    m_menuEmpty->addAction(m_uploadAction);

    connect(m_downloadAction,&QAction::triggered,[=]()
            {
                addDownloadFiles();
            });
    connect(m_delAction,&QAction::triggered,[=]()
            {
                dealSelectdFile("删除");
            });
    connect(m_shareAction,&QAction::triggered,[=]()
            {
                dealSelectdFile("分享");
            });
    connect(m_propertyAction,&QAction::triggered,[=]()
            {
                dealSelectdFile("属性");
            });
    connect(m_pvAscendingAction,&QAction::triggered,[=]()
            {
                refreshFiles(PvAsc);
            });
    connect(m_pvDescendingAction,&QAction::triggered,[=]()
            {
                refreshFiles(PvDesc);
            });
    connect(m_refreshAction,&QAction::triggered,[=]()
            {
                refreshFiles();
            });
    connect(m_uploadAction,&QAction::triggered,[=]()
            {
                addUploadFiles();
            });
}

//==========>上传文件处理<==============
// 添加需要上传的文件到上传任务列表
void filewg::addUploadFiles()
{
    QStringList path=QFileDialog::getOpenFileNames(this, tr("Select one or more files to upload"),"./", "file(*.*)");
    
    emit gotoTransfer(Upload);
    
    UploadTask *uploadtask=UploadTask::getInstance();  

    for (int i=0;i<path.size();i++)
    {
        int ret=uploadtask->appendUploadList(path.at(i));

        if(ret==0)
        {
            cout<<"文件成功添加到上传列表"<<endl;
        }
        else
        {
           cout<<"文件添加到上传列表失败"<<endl;
           return;
        }
    }

    return;

}
// 设置md5信息的json包
QByteArray filewg::setMd5Json(QString user, QString token, QString md5, QString fileName)
{
    QMap<QString,QVariant> filejson;
    filejson.insert("user",user);
    filejson.insert("token",token);
    filejson.insert("md5",md5);
    filejson.insert("fileName",fileName);

    QJsonDocument doc=QJsonDocument::fromVariant(filejson);
    return doc.toJson();
}
// 上传文件处理，取出上传任务列表的队首任务，上传完后，再取下一个任务
void filewg::uploadFilesAction()
{
    UploadTask *uploadtask=UploadTask::getInstance();

    if(uploadtask->isEmpty())
        return;

    if(uploadtask->isUpload())
        return;

    UploadFileInfo* uploadfileinfo=uploadtask->takeTask();

    LoginInfoInstance *login=LoginInfoInstance::getInstance();
    QByteArray filejson=setMd5Json(login->getUser(),login->getToken(),uploadfileinfo->md5,uploadfileinfo->fileName);

    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    request.setHeader(QNetworkRequest::ContentLengthHeader,filejson.size());

    QString url=QString("http://%1:%2/md5").arg(login->getIp()).arg(login->getPort());
    request.setUrl(QUrl(url));

    QNetworkReply *reply=m_manager->post(request,filejson);
    connect(reply,&QNetworkReply::readyRead,[=]()
            {
                QByteArray data=reply->readAll();

                QString status=m_cm.getCode(data);

                if("007"==status)
                {
                    cout<<uploadfileinfo->fileName<<"秒传失败"<<endl;
                    uploadFile(uploadfileinfo);
                }
                else if("006"==status)
                {
                    cout<<uploadfileinfo->fileName<<"秒传成功"<<endl;
                    uploadtask->dealUploadTask();
                }
                else if("005"==status)
                {
                    cout<<uploadfileinfo->fileName<<"文件已经存在"<<endl;
                    uploadtask->dealUploadTask();
                }
                else
                {
                    emit loginAgainSignal();
                }

                reply->deleteLater();
            });


}
// 上传真正的文件内容，不能秒传的前提下
void filewg::uploadFile(UploadFileInfo *info)
{
    LoginInfoInstance *login=LoginInfoInstance::getInstance();
    QString boundary=m_cm.getBoundary();

    QByteArray data;

    data.append(boundary);
    data.append("\r\n");

    data.append("Content-Disposition: form-data; ");
    data.append( QString("user=\"%1\" ").arg( login->getUser() ) ); //上传用户
    data.append( QString("filename=\"%1\" ").arg(info->fileName) ); //文件名字
    data.append( QString("md5=\"%1\" ").arg(info->md5) ); //文件md5码
    data.append( QString("size=%1").arg(info->size)  );   //文件大小
    data.append("\r\n");

    data.append("Content-Type: multipart/form-data");
    data.append("\r\n");
    data.append("\r\n");

    data.append(info->file->readAll());
    data.append("\r\n");

    data.append(boundary);

    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader,"multipart/form-data");

    QString url=QString("http://%1:%2/upload").arg(login->getIp()).arg(login->getPort());
    request.setUrl(QUrl(url));

    QNetworkReply *reply=m_manager->post(request,data);

    connect(reply,&QNetworkReply::uploadProgress,[=](qint64 bytesSent, qint64 bytesTotal)
            {
                if(bytesTotal!=0)
                    info->dp->setProgress(bytesSent/1024,bytesTotal/1024);
            });

    connect(reply,&QNetworkReply::readyRead,[=]()
            {

                if(reply->error()!=QNetworkReply::NoError)
                {
                    cout<<reply->errorString();
                    reply->deleteLater();
                    return;
                }

                QByteArray data=reply->readAll();
                reply->deleteLater();
                if("008" == m_cm.getCode(data) )
                {
                    cout<<info->fileName<<"龟速上传成功"<<endl;
                    m_cm.writeRecord(login->getUser(), info->fileName, "008");
                }
                else if("009" == m_cm.getCode(data) )
                {
                    cout<<info->fileName<<"龟速上传失败"<<endl;
                    m_cm.writeRecord(login->getUser(), info->fileName, "009");
                }

                QString status=m_cm.getCode(data);

                UploadTask *task=UploadTask::getInstance();
                if(task==nullptr)
                {
                    cout<<"UploadTask::getInstance() ==NULL"<<endl;
                    return;
                }

                task->dealUploadTask();
            });

    return;
}

//==========>文件item展示<==============
// 清空文件列表
void filewg::clearFileList()
{
    int n=m_fileList.size();
//    for(int i=0;i<m_fileList.size();i++)
//    错误写法,应将m_fileList.size()存储起来,因每次循环使用takefirst()改变了m_fileList大小导致最后循环次数减小,无法将m_fileList清空
    for(int i=0;i<n;i++)
    {
        FileInfo *info=m_fileList.takeFirst();
        delete info;
    }
}
// 清空所有item项目
void filewg::clearItems()
{
    int n=ui->listWidget->count();
    cout<<"ui->listWidget->count():"<<ui->listWidget->count();
    for(int i=0;i<n;i++)
    {
        QListWidgetItem *item=ui->listWidget->takeItem(0);
        delete item;
    }

}
// 添加上传文件项目item
void filewg::addUploadItem(QString iconPath, QString name)
{
    QListWidgetItem *UploadItem=new QListWidgetItem(QIcon(iconPath),name);
    ui->listWidget->addItem(UploadItem);
}
// 文件item展示
void filewg::refreshFileItems()
{
    clearItems();

    if(!m_fileList.isEmpty())
    {
        cout<<"m_fileList.size():"<<m_fileList.size();
        for(int i=0;i<m_fileList.size();i++)
        {
            ui->listWidget->addItem(m_fileList.at(i)->item);
        }
    }

    this->addUploadItem();
}

//==========>显示用户的文件列表<==============
// desc是descend 降序意思
// asc 是ascend 升序意思
// Normal：普通用户列表，PvAsc：按下载量升序， PvDesc：按下载量降序
//enum Display{Normal, PvAsc, PvDesc};
// 得到服务器json文件
QStringList filewg::getCountStatus(QByteArray json)
{
    QStringList list;

    QJsonDocument doc=QJsonDocument::fromJson(json);
    if(doc.isObject())
    {
        QJsonObject obj=doc.object();
        QString num=obj.value("num").toString();
        QString code=obj.value("code").toString();

        list.append(code);
        list.append(num);
    }

    return list;
}
// 显示用户的文件列表
void filewg::refreshFiles(Display cmd)
{
    LoginInfoInstance *login=LoginInfoInstance::getInstance();

    QByteArray CountJson=setGetCountJson(login->getUser(),login->getToken());

    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    request.setHeader(QNetworkRequest::ContentLengthHeader,CountJson.size());

    QString url=QString("http://%1:%2/myfiles?cmd=count").arg(login->getIp()).arg(login->getPort());
    request.setUrl(QUrl(url));

    QNetworkReply *reply=m_manager->post(request,CountJson);

    connect(reply,&QNetworkReply::readyRead,[=]()
            {
                QByteArray data=reply->readAll();

                reply->deleteLater();

                QStringList status=getCountStatus(data);

                cout<<status<<endl;

                clearFileList();
                cout<<"refreshFiles::m_filelistsize:"<<m_fileList.size();

                if("110"==status.at(0))
                {
                    m_userFilesCount=status.at(1).toLong();

                    if(m_userFilesCount>0)
                    {
                        m_start=0;
                        m_count=10;
                        getUserFilesList(cmd);
                    }
                    else
                    {
                        refreshFileItems();
                    }

                }
                else
                {
                    QMessageBox::information(this,"用户","用户登陆验证码失效,请重新登录");
                    emit loginAgainSignal();
                }

            });

}
// 设置json包
QByteArray filewg::setGetCountJson(QString user, QString token)
{
    QMap<QString,QVariant> CountJson;
    CountJson.insert("user",user);
    CountJson.insert("token",token);

    QJsonDocument doc=QJsonDocument::fromVariant(CountJson);
    return doc.toJson();
}
// 设置json包
QByteArray filewg::setFilesListJson(QString user, QString token, int start, int count)
{
    QMap<QString,QVariant> CountJson;
    CountJson.insert("user",user);
    CountJson.insert("token",token);
    CountJson.insert("start",start);
    CountJson.insert("count",count);

    QJsonDocument doc=QJsonDocument::fromVariant(CountJson);
    return doc.toJson();
}
// 获取用户文件列表
void filewg::getUserFilesList(Display cmd)
{   
    if(m_userFilesCount ==0)
    {
        cout << "获取用户文件列表条件结束";
        refreshFileItems();
        return;
    }

    if(m_userFilesCount < m_count)
        m_count=m_userFilesCount;

    LoginInfoInstance *login=LoginInfoInstance::getInstance();

    QByteArray CountJson=setFilesListJson(login->getUser(),login->getToken(),m_start,m_count);

    m_start+=m_count;
    m_userFilesCount-=m_count;

    QString tmp;

    if(cmd==Normal)
        tmp="normal";
    else if(cmd==PvAsc)
        tmp="pvasc";
    else
        tmp="pvdesc";

    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    request.setHeader(QNetworkRequest::ContentLengthHeader,CountJson.size());

    QString url=QString("http://%1:%2/myfiles?cmd=%3").arg(login->getIp()).arg(login->getPort()).arg(tmp);
    request.setUrl(QUrl(url));

    QNetworkReply *reply=m_manager->post(request,CountJson);
    if(reply==nullptr)
    {
        cout<<"reply==nullptr";
        return;
    }

    connect(reply,&QNetworkReply::finished,[=]()
            {
                QByteArray data=reply->readAll();

                reply->deleteLater();

                if("111" == m_cm.getCode(data) ) //common.h
                {
                    QMessageBox::warning(this, "账户异常", "请重新登陆！！！");
                    emit loginAgainSignal(); //发送重新登陆信号

                    return; //中断
                }

                //不是错误码就处理文件列表json信息
                if("015" != m_cm.getCode(data) ) //common.h
                {
                    getFileJsonInfo(data);//解析文件列表json信息，存放在文件列表中

                    //继续获取用户文件列表
                    getUserFilesList(cmd);

                    cout<<"getUserFilesList::m_fileList.size():"<<m_fileList.size();
                }

            });
}
// 解析文件列表json信息，存放在文件列表中
void filewg::getFileJsonInfo(QByteArray data)
{
    QJsonDocument doc=QJsonDocument::fromJson(data);

    if(doc.isObject())
    {
        QJsonObject obj=doc.object();
        QJsonArray  fileArray=obj.value("files").toArray();

        for(int i=0;i<fileArray.size();i++)
        {
//            {
//            "user": "yoyo",
//            "md5": "e8ea6031b779ac26c319ddf949ad9d8d",
//            "time": "2017-02-26 21:35:25",
//            "filename": "test.mp4",
//            "share_status": 0,
//            "pv": 0,
//            "url": "http://192.168.31.109:80/group1/M00/00/00/wKgfbViy2Z2AJ-FTAaM3As-g3Z0782.mp4",
//            "size": 27473666,
//             "type": "mp4"
//            }
            FileInfo *fileinfo=new FileInfo;           
            cout<<fileArray[i].toObject().value("size");
            fileinfo->user=fileArray[i].toObject().value("user").toString();
            fileinfo->md5=fileArray[i].toObject().value("md5").toString();
            fileinfo->time=fileArray[i].toObject().value("time").toString();
            fileinfo->filename=fileArray[i].toObject().value("filename").toString();
            fileinfo->shareStatus=fileArray[i].toObject().value("share_status").toInt();
            fileinfo->pv=fileArray[i].toObject().value("pv").toInt();
            fileinfo->url=fileArray[i].toObject().value("url").toString();
            fileinfo->size=qint64(fileArray[i].toObject().value("size").toDouble());
            fileinfo->type=fileArray[i].toObject().value("type").toString();

            QString filepath=m_cm.getFileType(fileinfo->type+".png");
            fileinfo->item=new QListWidgetItem(QIcon(filepath),fileinfo->filename);

            cout<<fileinfo->user<<" "<<fileinfo->md5<<" "<<fileinfo->time<<" "<<fileinfo->url<<" "<<fileinfo->type<<" "<<fileinfo->size;
            m_fileList.append(fileinfo);

        }
    }
}

//==========>分享、删除文件<==============
// 处理选中的文件
void filewg::dealSelectdFile(QString cmd)
{
    QListWidgetItem *item=ui->listWidget->currentItem();
    FileInfo *info=nullptr;

    for(int i=0;i<m_fileList.size();i++)
    {
        if(m_fileList.at(i)->item == item)
        {
            info=m_fileList.at(i);
            break;
        }
    }

    if(cmd =="分享")
    {
        shareFile(info);
    }
    else if (cmd == "删除")
    {
        delFile(info);
    }
    else if (cmd == "属性")
    {
        getFileProperty(info);
    }

}
QByteArray filewg::setDealFileJson(QString user, QString token, QString md5, QString filename)//设置json包
{
    QMap<QString,QVariant> filejson;
    filejson.insert("user",user);
    filejson.insert("token",token);
    filejson.insert("md5",md5);
    filejson.insert("filename",filename);

    QJsonDocument doc=QJsonDocument::fromVariant(filejson);
    return doc.toJson();
}
//==========>分享文件<==============
void filewg::shareFile(FileInfo *info)//分享某个文件
{
    if(info->shareStatus == 1) //已经分享，不能再分享
    {
        QMessageBox::warning(this, "此文件已经分享", "此文件已经分享!!!");
        return;
    }

    LoginInfoInstance *login=LoginInfoInstance::getInstance();

    QByteArray data=setDealFileJson(login->getUser(), login->getToken() ,info->md5, info->filename);

    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    request.setHeader(QNetworkRequest::QNetworkRequest::ContentLengthHeader,data.size());

    QString url=QString("http://%1:%2/dealfile?cmd=share").arg(login->getIp()).arg(login->getPort());
    request.setUrl(QUrl(url));

    QNetworkReply *reply=m_manager->post(request,data);

    connect(reply,&QNetworkReply::readyRead,[=]()
            {

                if(reply->error()!=QNetworkReply::NoError)
                {
                    cout<<reply->errorString();
                    reply->deleteLater();
                    return;
                }

                QByteArray data=reply->readAll();
                reply->deleteLater();

                if("010" == m_cm.getCode(data) )
                {
                    cout<<info->filename<<"sharefile成功";

                    for(int i=0;i<m_fileList.size();i++)
                    {
                        if(m_fileList.at(i)->md5 == info->md5 && m_fileList.at(i)->filename ==info->filename)
                        {
                            m_fileList.at(i)->shareStatus=1;

                            break;
                        }

                    }
                }
                else if("011" == m_cm.getCode(data) )
                {
                    QMessageBox::information(this,tr("分享文件"),tr("文件分享失败"));
                }
                else if("012" == m_cm.getCode(data) )
                {
                    QMessageBox::information(this,tr("分享文件"),tr("文件已经被其他用户分享存储在云盘"));
                }
                else
                {
                    QMessageBox::information(this,tr("用户"),tr("用户登陆验证码失效,请重新登录"));
                    emit loginAgainSignal();
                }

            });

    return;
}
//==========>删除文件<==============
void filewg::delFile(FileInfo *info)//删除某个文件
{
    LoginInfoInstance *login=LoginInfoInstance::getInstance();

    QByteArray data=setDealFileJson(login->getUser(), login->getToken() ,info->md5, info->filename);

    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    request.setHeader(QNetworkRequest::QNetworkRequest::ContentLengthHeader,data.size());

    QString url=QString("http://%1:%2/dealfile?cmd=del").arg(login->getIp()).arg(login->getPort());
    request.setUrl(QUrl(url));

    QNetworkReply *reply=m_manager->post(request,data);

    connect(reply,&QNetworkReply::readyRead,[=]()
            {

                if(reply->error()!=QNetworkReply::NoError)
                {
                    cout<<reply->errorString();
                    reply->deleteLater();
                    return;
                }

                QByteArray data=reply->readAll();
                reply->deleteLater();

                if("013" == m_cm.getCode(data) )
                {
                    cout<<info->filename<<"deletefile成功";

                    for(int i=0;i<m_fileList.size();i++)
                    {
                        if(m_fileList.at(i)->md5 == info->md5 && m_fileList.at(i)->filename ==info->filename)
                        {
                            FileInfo *info=m_fileList.takeAt(i);
                            QListWidgetItem *item=info->item;
                            ui->listWidget->removeItemWidget(item);
                            delete  item;
                            delete  info;

                            break;
                        }

                    }
                }
                else if("014" == m_cm.getCode(data) )
                {
                    cout<<info->filename<<"deletefile失败";
                }
                else
                {
                    QMessageBox::information(this,tr("用户"),tr("用户登陆验证码失效,请重新登录"));
                    emit loginAgainSignal();
                }

            });

    return;
}
//==========>获取文件属性<==============
void filewg::getFileProperty(FileInfo *info)//获取属性信息
{
    FilePropertyInfo *filepropertyinfo=new FilePropertyInfo(this);
    filepropertyinfo->setInfo(info);
    filepropertyinfo->show();

    connect(filepropertyinfo,&FilePropertyInfo::close,
            [=]()
            {
                delete filepropertyinfo;
            });

}
//==========>下载文件处理<==============
// 添加需要下载的文件到下载任务列表
void filewg::addDownloadFiles()
{
    QString path;

    emit gotoTransfer(Download);

    DownloadTask *downloadtask=DownloadTask::getInstance();

    QListWidgetItem *item=ui->listWidget->currentItem();
    FileInfo *info=nullptr;

    for(int i=0;i<m_fileList.size();i++)
    {
        if(m_fileList.at(i)->item == item)
        {
            info=m_fileList.at(i);
            path=QFileDialog::getSaveFileName(this, tr("Select one or more files to upload"), m_fileList.at(i)->filename);
            break;
        }
    }

    downloadtask->appendDownloadList(info,path);

    return;
}
//下载文件处理，取出下载任务列表的队首任务，下载完后，再取下一个任务
void filewg::downloadFilesAction()
{
    DownloadTask *downloadtask=DownloadTask::getInstance();
    if(downloadtask==nullptr)
    {
        cout<<"DownloadTask::getInstance()==nullptr";
        return;
    }

    if(downloadtask->isEmpty())
        return;

    if(downloadtask->isDownload())
        return;

    if(downloadtask->isShareTask())
        return;

    DownloadInfo* downloadfileinfo=downloadtask->takeTask();
//    QFile *file;        //文件指针
//    QString user;       //下载用户
//    QString filename;   //文件名字
//    QString md5;        //文件md5
//    QUrl url;           //下载网址
//    bool isDownload;    //是否已经在下载
//    DataProgress *dp;   //下载进度控件
//    bool isShare;       //是否为共享文件下载

    LoginInfoInstance *login=LoginInfoInstance::getInstance();

    QNetworkReply *reply=m_manager->get(QNetworkRequest(downloadfileinfo->url));

    connect(reply,&QNetworkReply::finished,[=]()
            {
                QByteArray data=reply->readAll();

                downloadfileinfo->file->write(data);

                cout<<downloadfileinfo->filename<<"下载成功"<<endl;
                m_cm.writeRecord(login->getUser(), downloadfileinfo->filename, "010");

                dealFilePv(downloadfileinfo->md5,downloadfileinfo->filename);

                downloadtask->dealDownloadTask();

                reply->deleteLater();
            });

    connect(reply,&QNetworkReply::downloadProgress,[=](qint64 bytesReceived, qint64 bytesTotal)
            {
                if(bytesTotal!=0)
                    downloadfileinfo->dp->setProgress(bytesReceived/1024,bytesTotal/1024);

            });
}
//==========>下载文件标志处理<==============
void filewg::dealFilePv(QString md5, QString filename) //下载文件pv字段处理
{
    LoginInfoInstance *login=LoginInfoInstance::getInstance();

    QByteArray data=setDealFileJson(login->getUser(), login->getToken() ,md5, filename);

    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    request.setHeader(QNetworkRequest::QNetworkRequest::ContentLengthHeader,data.size());

    QString url=QString("http://%1:%2/dealfile?cmd=pv").arg(login->getIp()).arg(login->getPort());
    request.setUrl(QUrl(url));

    QNetworkReply *reply=m_manager->post(request,data);

    connect(reply,&QNetworkReply::readyRead,[=]()
            {

                if(reply->error()!=QNetworkReply::NoError)
                {
                    cout<<reply->errorString();
                    reply->deleteLater();
                    return;
                }

                QByteArray data=reply->readAll();
                reply->deleteLater();

                if("016" == m_cm.getCode(data) )
                {
                    cout<<filename<<"dealFilePv成功";

                    for(int i=0;i<m_fileList.size();i++)
                    {
                        if(m_fileList.at(i)->md5 == md5 && m_fileList.at(i)->filename == filename)
                        {
                            m_fileList.at(i)->pv+=1;

                            break;
                        }
                    }
                }
                else if("017" == m_cm.getCode(data) )
                {
                    cout<<filename<<"dealFilePv失败";
                }
                else
                {
                    QMessageBox::information(this,"用户","用户登陆验证码失效,请重新登录");
                    emit loginAgainSignal();
                }

            });

    return;
}
//清除上传下载任务
void filewg::clearAllTask()
{
    UploadTask *upTask=UploadTask::getInstance();
    if(upTask == nullptr)
    {
        cout << "UploadTask::getInstance() == NULL";
        return;
    }

    upTask->clearList();

    DownloadTask *downTask=DownloadTask::getInstance();
    if(downTask == nullptr)
    {
        cout << "UploadTask::getInstance() == NULL";
        return;
    }

    downTask->clearList();

}
// 定时检查处理任务队列中的任务
void filewg::checkTaskList()
{
    m_uploadFileTimer.start(500);
    connect(&m_uploadFileTimer,&QTimer::timeout,[=]()
            {
               uploadFilesAction();
            });
    m_downloadTimer.start(500);
    connect(&m_downloadTimer,&QTimer::timeout,[=]()
            {
               downloadFilesAction();
            });
}

//右键菜单
void filewg::rightMenu(const QPoint &pos)
{
    QListWidgetItem *item=ui->listWidget->itemAt(pos);

    if(item==nullptr)
    {
        m_menuEmpty->exec(QCursor::pos());
    }
    else
    {
        ui->listWidget->setCurrentItem(item);
        if(item->text()=="上传文件")
            return;
        m_menu->exec(QCursor::pos());
    }
}
