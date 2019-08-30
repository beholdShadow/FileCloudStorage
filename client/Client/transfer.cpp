#include "transfer.h"
#include "ui_transfer.h"
#include "common/downloadlayout.h"
#include "common/uploadlayout.h"
#include "common/logininfoinstance.h"
#include <QFile>

transfer::transfer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::transfer)
{
    ui->setupUi(this);

    UploadLayout *uploadlayout=UploadLayout::getInstance();
    uploadlayout->setUploadLayout(ui->upload_scroll);

    DownloadLayout* downloadlayout=DownloadLayout::getInstance();
    downloadlayout->setDownloadLayout(ui->download_scroll);

    ui->tabWidget->setCurrentIndex(0);

    connect(ui->tabWidget,&QTabWidget::currentChanged,[=](int index)
            {
                if(index == 2)
                {
                    dispayDataRecord();
                }
            });

    ui->tabWidget->tabBar()->setStyleSheet(
       "QTabBar::tab{"
       "background-color: rgb(182, 202, 211);"
       "border-right: 1px solid gray;"
       "padding: 6px"
       "}"
       "QTabBar::tab:selected, QtabBar::tab:hover {"
       "background-color: rgb(20, 186, 248);"
       "}"
    );
}

transfer::~transfer()
{
    delete ui;
}

void transfer::dispayDataRecord(QString path)
{
    LoginInfoInstance *login=LoginInfoInstance::getInstance();

   QFile file(path+login->getUser());
   if(!file.open(QIODevice::ReadOnly))
   {
       cout<<"dispayDataRecord open file error";
       return;
   }

   QByteArray data=file.readAll();
   cout<<data;
   ui->textEdit->setText(QString::fromLocal8Bit(data));

   file.close();
}

void transfer::showUpload()
{
    ui->tabWidget->setCurrentWidget(ui->upload);
}

void transfer::showDownload()
{
    ui->tabWidget->setCurrentWidget(ui->download);
}

void transfer::on_toolButton_clicked()
{
    LoginInfoInstance *login=LoginInfoInstance::getInstance();

    QString file=RECORDDIR+login->getUser();
    cout<<file;
    if(QFile::exists(file))
    {
        QFile::remove(file);
        ui->textEdit->clear();
    }
}
