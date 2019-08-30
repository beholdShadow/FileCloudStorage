#include "sharelist.h"
#include "ui_sharelist.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QMessageBox>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QFileDialog>
#include "common/logininfoinstance.h"
#include "common/downloadtask.h"
#include "selfwidget/filepropertyinfo.h"

sharelist::sharelist(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::sharelist)
{
    ui->setupUi(this);

    initListWidget();

    addActionMenu();

    m_manager=m_cm.getNetManager();

    m_downloadTimer.start(500);

    connect(&m_downloadTimer,&QTimer::timeout,
            [=]()
            {
                downloadFilesAction();
            });

}

sharelist::~sharelist()
{
    delete ui;
}

void sharelist::initListWidget()
{
    ui->listWidget->setViewMode(QListView::IconMode);
    ui->listWidget->setIconSize(QSize(80,80));
    ui->listWidget->setGridSize(QSize(100,120));

    ui->listWidget->setResizeMode(QListView::Adjust);

    ui->listWidget->setMovement(QListView::Static);

    ui->listWidget->setSpacing(30);

    ui->listWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->listWidget,&QListWidget::customContextMenuRequested,
            [=](const QPoint pos)
            {

                QListWidgetItem *item=ui->listWidget->itemAt(pos);

                if(item==nullptr)
                {
                    m_menuEmpty->exec(QCursor::pos());
                }
                else
                {
                    ui->listWidget->setCurrentItem(item);
                    m_menuItem->exec(QCursor::pos());
                }

            });

}

void sharelist::addActionMenu()
{
    m_menuItem=new MyMenu(this);
    m_downloadAction=new QAction("下载",this);
    m_propertyAction=new QAction("属性",this);
    m_cancelAction=new QAction("取消分享",this);
    m_saveAction=new QAction("转存文件",this);
    m_menuItem->addAction(m_downloadAction);
    m_menuItem->addAction(m_propertyAction);
    m_menuItem->addAction(m_cancelAction);
    m_menuItem->addAction(m_saveAction);

    m_menuEmpty=new MyMenu(this);
    m_refreshAction=new QAction("刷新",this);
    m_menuEmpty->addAction(m_refreshAction);

    connect(m_downloadAction,&QAction::triggered,[=]()
            {
                addDownloadFiles();
            });
    connect(m_propertyAction,&QAction::triggered,[=]()
            {
                dealSelectdFile(Property);
            });
    connect(m_cancelAction,&QAction::triggered,[=]()
            {
                dealSelectdFile(Cancel);
            });
    connect(m_saveAction,&QAction::triggered,[=]()
            {
                dealSelectdFile(Save);
            });
    connect(m_refreshAction,&QAction::triggered,[=]()
            {
                refreshFiles();
            });

}

void sharelist::clearshareFileList()
{
    int n=m_shareFileList.size();

    for(int i=0;i<n;i++)
    {
        FileInfo *tmp=m_shareFileList.takeAt(0);
        delete  tmp;
    }

}

void sharelist::clearItems()
{
    int n=ui->listWidget->count();

    for(int i=0;i<n;i++)
    {
        QListWidgetItem *item=ui->listWidget->takeItem(0);
        delete  item;
    }

}

void sharelist::refreshFileItems()
{

    clearItems();

    for(int i=0;i<m_shareFileList.size();i++)
    {
        ui->listWidget->addItem(m_shareFileList.at(i)->item);
    }

}

void sharelist::refreshFiles()
{
    LoginInfoInstance *login=LoginInfoInstance::getInstance();

    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");

    QString url=QString("http://%1:%2/sharefiles?cmd=count").arg(login->getIp()).arg(login->getPort());
    request.setUrl(QUrl(url));

    QNetworkReply *reply=m_manager->post(request,"");

    connect(reply,&QNetworkReply::readyRead,[=]()
            {
                QByteArray data=reply->readAll();

                reply->deleteLater();

                m_userFilesCount=data.toInt();

                clearshareFileList();

                cout<<"refreshFiles::m_shareFileList:"<<m_shareFileList.size();

                if(m_userFilesCount>0)
                {
                    m_start=0;
                    m_count=10;

                    getUserFilesList();

                }
                else
                {
                    refreshFileItems();
                }

            });
}

QByteArray sharelist::setFilesListJson(int start, int count)
{
    QMap<QString,QVariant> CountJson;
    CountJson.insert("start",start);
    CountJson.insert("count",count);

    QJsonDocument doc=QJsonDocument::fromVariant(CountJson);
    return doc.toJson();
}

void sharelist::getUserFilesList()
{
    if(m_userFilesCount ==0)
    {
        cout << "获取共享文件列表条件结束";
        refreshFileItems();
        return;
    }

    if(m_userFilesCount < m_count)
        m_count=m_userFilesCount;

    LoginInfoInstance *login=LoginInfoInstance::getInstance();

    QByteArray CountJson=setFilesListJson(m_start,m_count);

    m_start+=m_count;
    m_userFilesCount-=m_count;

    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    request.setHeader(QNetworkRequest::ContentLengthHeader,CountJson.size());

    QString url=QString("http://%1:%2/sharefiles?cmd=normal").arg(login->getIp()).arg(login->getPort());
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

                if("015" != m_cm.getCode(data) ) //common.h
                {
                    getFileJsonInfo(data);//解析文件列表json信息，存放在文件列表中

                    //继续获取共享列表
                    getUserFilesList();

                }
                else
                {
                    cout<<"共享列表获取失败";
                    return;
                }

            });
}

void sharelist::getFileJsonInfo(QByteArray data)
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

            cout<<fileinfo->user<<" "<<fileinfo->md5<<" "<<fileinfo->time<<" "<<fileinfo->url<<" "<<fileinfo->type;
            m_shareFileList.append(fileinfo);

        }
    }
}

void sharelist::addDownloadFiles()
{
    QString path;

    emit gotoTransfer(Download);

    DownloadTask *downloadtask=DownloadTask::getInstance();

    QListWidgetItem *item=ui->listWidget->currentItem();
    FileInfo *info=nullptr;

    for(int i=0;i<m_shareFileList.size();i++)
    {
        if(m_shareFileList.at(i)->item == item)
        {
            info=m_shareFileList.at(i);
            path=QFileDialog::getSaveFileName(this, tr("Select one or more files to upload"), m_shareFileList.at(i)->filename);
            break;
        }
    }

    downloadtask->appendDownloadList(info,path,true);

    return;
}

void sharelist::downloadFilesAction()
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

    if(!downloadtask->isShareTask())
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

QByteArray sharelist::setShareFileJson(QString user, QString md5, QString filename)
{
    QMap<QString,QVariant> ShareFileJson;
    ShareFileJson.insert("user",user);
    ShareFileJson.insert("md5",md5);
    ShareFileJson.insert("filename",filename);

    QJsonDocument doc=QJsonDocument::fromVariant(ShareFileJson);
    return doc.toJson();
}

void sharelist::dealFilePv(QString md5, QString filename)
{
    LoginInfoInstance *login=LoginInfoInstance::getInstance();

    QByteArray data=setShareFileJson(login->getUser(),md5, filename);

    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    request.setHeader(QNetworkRequest::QNetworkRequest::ContentLengthHeader,data.size());

    QString url=QString("http://%1:%2/dealsharefile?cmd=pv").arg(login->getIp()).arg(login->getPort());
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

                    for(int i=0;i<m_shareFileList.size();i++)
                    {
                        if(m_shareFileList.at(i)->md5 == md5 && m_shareFileList.at(i)->filename == filename)
                        {
                            m_shareFileList.at(i)->pv+=1;

                            break;
                        }
                    }
                }
                else if("017" == m_cm.getCode(data) )
                {
                    cout<<filename<<"dealFilePv失败";
                }
            });

    return;
}

void sharelist::dealSelectdFile(sharelist::CMD cmd)
{
    QListWidgetItem *item=ui->listWidget->currentItem();
    FileInfo *tmp=nullptr;

    for(int i=0;i<m_shareFileList.size();i++)
    {
        if(m_shareFileList.at(i)->item==item)
        {
            tmp=m_shareFileList.at(i);
            break;
        }
    }

    if(cmd == Property)
        getFileProperty(tmp);
    else if(cmd == Cancel)
        cancelShareFile(tmp);
    else if(cmd == Save)
        saveFileToMyList(tmp);

}

void sharelist::getFileProperty(FileInfo *info)
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

void sharelist::cancelShareFile(FileInfo *info)
{
    LoginInfoInstance *login=LoginInfoInstance::getInstance();

    if(info->user != login->getUser()) //别人分享的文件，你无法取消分享
    {
        QMessageBox::warning(this, "文件无法取消分享", "此文件不是当前登陆用户分享，无法取消分享！！！");
        return;
    }

    QByteArray data=setShareFileJson(login->getUser(),info->md5, info->filename);

    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    request.setHeader(QNetworkRequest::QNetworkRequest::ContentLengthHeader,data.size());

    QString url=QString("http://%1:%2/dealsharefile?cmd=cancel").arg(login->getIp()).arg(login->getPort());
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

                if("018" == m_cm.getCode(data) )
                {
                    QMessageBox::information(this, "操作成功", "此文件已取消分享！！！");

                    for(int i=0;i<m_shareFileList.size();i++)
                    {
                        if(m_shareFileList.at(i)->md5 == info->md5 && m_shareFileList.at(i)->filename == info->filename)
                        {
                            FileInfo *info=m_shareFileList.takeAt(i);
                            QListWidgetItem *item=info->item;
                            ui->listWidget->removeItemWidget(item);
                            delete  item;
                            delete  info;

                            break;
                        }
                    }
                }
                else if("019" == m_cm.getCode(data) )
                {
                    QMessageBox::warning(this, "操作失败", "取消分享文件操作失败！！！");
                }
            });

    return;
}

void sharelist::saveFileToMyList(FileInfo *info)
{

    LoginInfoInstance *login=LoginInfoInstance::getInstance();

    QByteArray data=setShareFileJson(login->getUser(),info->md5, info->filename);

    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    request.setHeader(QNetworkRequest::QNetworkRequest::ContentLengthHeader,data.size());

    QString url=QString("http://%1:%2/dealsharefile?cmd=save").arg(login->getIp()).arg(login->getPort());
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

                if("020" == m_cm.getCode(data) )
                {
                    QMessageBox::information(this, "操作成功", "此文件转存成功！！！");
                }
                else if("021" == m_cm.getCode(data) )
                {
                    QMessageBox::information(this, "操作失败", "此文件已经存在用户！！！");
                }
                else if("022" == m_cm.getCode(data) )
                {
                    QMessageBox::warning(this, "操作失败", "转存文件操作失败！！！");
                }

            });

    return;

}
