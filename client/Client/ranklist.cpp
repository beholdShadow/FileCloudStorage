#include "ranklist.h"
#include "ui_ranklist.h"
#include "common/logininfoinstance.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

ranklist::ranklist(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ranklist)
{
    ui->setupUi(this);

    m_manager=m_cm.getNetManager();

    initTableWidget();
}

ranklist::~ranklist()
{
    delete ui;
}

void ranklist::initTableWidget()
{
    ui->tableWidget->setColumnCount(3);
    ui->tableWidget->horizontalHeader()->setDefaultSectionSize(300);

    ui->tableWidget->horizontalHeader()->setSectionsClickable(false);

    QStringList header;
    header.append("排名");
    header.append("文件名");
    header.append("下载量");
    ui->tableWidget->setHorizontalHeaderLabels(header);

    QFont font = ui->tableWidget->horizontalHeader()->font(); // 获取表头原来的字体
    font.setBold(true);// 字体设置粗体
    ui->tableWidget->horizontalHeader()->setFont(font);

    ui->tableWidget->verticalHeader()->setDefaultSectionSize(40); // 设置处垂直方向高度
    // ui->tableWidget->setFrameShape(QFrame::NoFrame); // 设置无边框
    ui->tableWidget->setShowGrid(false); // 设置不显示格子线
    ui->tableWidget->verticalHeader()->setVisible(false); // 设置垂直头不可见，不自动显示行号
    ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);   // 单行选择
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers); // 设置不可编辑
    // ui->tableWidget->horizontalHeader()->resizeSection(0, 150); // 设置表头第一列的宽度为150
    // ui->tableWidget->horizontalHeader()->setFixedHeight(40);    // 设置表头的高度

    // 通过样式表，设置表头背景色
    ui->tableWidget->horizontalHeader()->setStyleSheet(
                "QHeaderView::section{"
                "background: skyblue;"
                "font: 16pt \"新宋体\";"
                "height: 35px;"
                "border:1px solid #c7f0ea;"
                "}");

    // ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    // 设置第0列的宽度
    ui->tableWidget->setColumnWidth(0,100);

    // 设置列宽策略，使列自适应宽度，所有列平均分来填充空白部分
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);

}

void ranklist::clearshareFileList()
{
    int n=m_list.size();
    for(int i=0;i<n;i++)
    {
        RankFileInfo *info=m_list.takeFirst();
        delete info;
    }
}

void ranklist::refreshFiles()
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
                cout<<m_userFilesCount;

                clearshareFileList();

                if(m_userFilesCount>0)
                {
                    m_start=0;
                    m_count=10;
                    getUserFilesList();
                }
                else
                {
                    refreshList();
                }
            });

}

QByteArray ranklist::setFilesListJson(int start, int count)
{
    QMap<QString,QVariant> FilesListJson;
    FilesListJson.insert("start",start);
    FilesListJson.insert("count",count);

    QJsonDocument doc=QJsonDocument::fromVariant(FilesListJson);
    return doc.toJson();
}

void ranklist::getUserFilesList()
{
    if(m_userFilesCount ==0)
    {
        cout << "获取下载榜文件列表条件结束";
        refreshList();
        return;
    }

    if(m_userFilesCount < m_count)
        m_count=m_userFilesCount;

    LoginInfoInstance *login=LoginInfoInstance::getInstance();

    QByteArray FilesListJson=setFilesListJson(m_start,m_count);

    m_start+=m_count;
    m_userFilesCount-=m_count;

    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    request.setHeader(QNetworkRequest::ContentLengthHeader,FilesListJson.size());

    QString url=QString("http://%1:%2/sharefiles?cmd=pvdesc").arg(login->getIp()).arg(login->getPort());
    request.setUrl(QUrl(url));

    QNetworkReply *reply=m_manager->post(request,FilesListJson);
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

                    //继续获取下载榜列表
                    getUserFilesList();

                }
                else
                {
                    cout<<"下载榜列表获取失败";
                    return;
                }

            });
}

void ranklist::getFileJsonInfo(QByteArray data)
{
    QJsonDocument doc=QJsonDocument::fromJson(data);

    if(doc.isObject())
    {
        QJsonObject obj=doc.object();
        QJsonArray  fileArray=obj.value("files").toArray();

        for(int i=0;i<fileArray.size();i++)
        {
//            {
//            "filename": "test.mp4",
//            "pv": 0,
//            }
            RankFileInfo *fileinfo=new RankFileInfo;

            fileinfo->filename=fileArray[i].toObject().value("filename").toString();
            fileinfo->pv=fileArray[i].toObject().value("pv").toInt();

            cout<<fileinfo->filename<<" "<<fileinfo->pv;
            m_list.append(fileinfo);

        }
    }
}

void ranklist::refreshList()
{
    int rowCount=ui->tableWidget->rowCount();
    for (int i=0;i<rowCount;i++)
    {
        ui->tableWidget->removeRow(0);
    }

    for(int i=0;i<m_list.size();i++)
    {
        QTableWidgetItem *item1=new QTableWidgetItem;
        QTableWidgetItem *item2=new QTableWidgetItem;
        QTableWidgetItem *item3=new QTableWidgetItem;

        ui->tableWidget->insertRow(i);

        item1->setTextAlignment(Qt::AlignHCenter |  Qt::AlignVCenter);
        item2->setTextAlignment(Qt::AlignLeft |  Qt::AlignVCenter);
        item3->setTextAlignment(Qt::AlignHCenter |  Qt::AlignVCenter);

        item1->setText(QString::number(i+1));
        item2->setText(m_list.at(i)->filename);
        item3->setText(QString::number(m_list.at(i)->pv));

        ui->tableWidget->setItem(i,0,item1);
        ui->tableWidget->setItem(i,1,item2);
        ui->tableWidget->setItem(i,2,item3);

    }
}
