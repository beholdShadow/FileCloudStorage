#include "mainwidget.h"
#include "ui_mainwidget.h"
#include <QPainter>

mainWidget::mainWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::mainWidget)
{
    ui->setupUi(this);

    ui->buttonGroup->setParent(this);

    managerSignals();

    ui->stackedWidget->setCurrentWidget(ui->myfiles);
}

mainWidget::~mainWidget()
{
    delete ui;
}

void mainWidget::showMainWindow()
{

    m_cm.moveToCenter(this);

    ui->stackedWidget->setCurrentWidget(ui->myfiles);

    ui->myfiles->refreshFiles();

}

void mainWidget::managerSignals()
{

     connect(ui->buttonGroup,&ButtonGroup::closeWindow,
             [=]()
             {
                 this->close();
             });
     connect(ui->buttonGroup,&ButtonGroup::minWindow,
             [=]()
             {
                 this->showMinimized();
             });
     connect(ui->buttonGroup,&ButtonGroup::maxWindow,
             [=]()
             {
                 if(this->isMaximized())
                     this->showNormal();
                 else
                     this->showMaximized();
             });
    connect(ui->buttonGroup,&ButtonGroup::sigMyFile,
            [=]()
            {
                ui->stackedWidget->setCurrentWidget(ui->myfiles);
                ui->myfiles->refreshFiles();
            });
    connect(ui->buttonGroup,&ButtonGroup::sigDownload,
            [=]()
            {
                ui->stackedWidget->setCurrentWidget(ui->rankerlist);
                ui->rankerlist->refreshFiles();
            });
    connect(ui->buttonGroup, &ButtonGroup::sigShareList, [=]()
            {
                ui->stackedWidget->setCurrentWidget(ui->sharefile);
                ui->sharefile->refreshFiles();
            });
    connect(ui->buttonGroup, &ButtonGroup::sigTransform,
            [=]()
            {
                ui->stackedWidget->setCurrentWidget(ui->transfer);
            });
    connect(ui->buttonGroup,&ButtonGroup::sigSwitchUser,
            [=]()
            {
                loginAgain();
            });
    connect(ui->myfiles,&filewg::loginAgainSignal,
            [=]()
            {
                loginAgain();
            });
    connect(ui->myfiles,&filewg::gotoTransfer,
            [=](TransferStatus status)
            {
                ui->buttonGroup->slotButtonClick(Page::TRANSFER);
                if(status == Download)
                {
                    ui->transfer->showDownload();
                }
                else if(status == Upload)
                {
                    ui->transfer->showUpload();
                }
            });
    connect(ui->sharefile,&sharelist::gotoTransfer,ui->myfiles,&filewg::gotoTransfer);

}

void mainWidget::loginAgain()
{
    emit changeUser();

    ui->myfiles->clearAllTask();
    ui->myfiles->clearFileList();
    ui->myfiles->clearItems();

}

void mainWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.drawPixmap(0,0,width(),height(),QPixmap(":/images/title_bk3.jpg"));
}
