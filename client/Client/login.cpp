#include "login.h"
#include "ui_login.h"
#include <QPainter>
#include <QMessageBox>
#include <QMouseEvent>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>

login::login(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::login)
{
    ui->setupUi(this);

    ui->passwd->setEchoMode(QLineEdit::Password);
    ui->registerPwd->setEchoMode(QLineEdit::Password);
    ui->registerRepeatPwd->setEchoMode(QLineEdit::Password);

    this->setWindowFlags(Qt::FramelessWindowHint);

    connect(ui->titleWg,&Titlewg::closeWindow,[=]()
            {
                if(ui->stackedWidget->currentWidget()==ui->set_page)
                    ui->stackedWidget->setCurrentWidget(ui->login_page);
                else if(ui->stackedWidget->currentWidget()==ui->reg_page)
                    ui->stackedWidget->setCurrentWidget(ui->login_page);
                else
                    this->close();
            });

    connect(ui->titleWg,&Titlewg::showSet,[=]()
            {
                ui->stackedWidget->setCurrentWidget(ui->set_page);
            });

    ui->stackedWidget->setCurrentWidget(ui->login_page);

    loadLoginConfig();

}

login::~login()
{
    delete ui;
}

void login::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.drawPixmap(0,0,width(),height(),QPixmap(":/images/login_bk.jpg"));
}
void login::mouseMoveEvent(QMouseEvent *ev)
{
   if(ev->buttons()&Qt::LeftButton)
       move(ev->globalPos()-p);
}

void login::mousePressEvent(QMouseEvent *ev)
{
    if(ev->button()==Qt::LeftButton)
        p=ev->globalPos()-this->geometry().topLeft();
}

QByteArray login::getRegisterJson(QString name, QString nickname, QString passwd, QString phone, QString email)
{
    QMap<QString,QVariant> registerinfo;
    registerinfo.insert("userName",name);
    registerinfo.insert("nickName",nickname);
    QString passwd_md5=QCryptographicHash::hash(passwd.toUtf8(),QCryptographicHash::Md5);
    registerinfo.insert("firstPwd",passwd_md5);
    registerinfo.insert("phone",phone);
    registerinfo.insert("email",email);

    QJsonDocument doc=QJsonDocument::fromVariant(registerinfo);
    return doc.toJson();
}

QByteArray login::getLoginJson(QString name, QString passwd)
{
    QMap<QString,QVariant> login;
    login.insert("user",name);

    QString passwd_md5=QCryptographicHash::hash(passwd.toUtf8(),QCryptographicHash::Md5);
    login.insert("pwd",passwd_md5);

    QJsonDocument doc=QJsonDocument::fromVariant(login);

    return doc.toJson();
}

QStringList login::parseLoginJson(QByteArray json)
{
    QStringList list;

    QJsonDocument doc=QJsonDocument::fromJson(json);
    if(doc.isObject())
    {
        QJsonObject obj=doc.object();
        list.append(obj.value("code").toString());
        list.append(obj.value("token").toString());
    }

    return list;
}

void login::loadLoginConfig()
{            
    QString user_base64= m_cm.getCfgValue("login", "user");
    QString pwd_base64= m_cm.getCfgValue("login", "pwd");
    QString remember = m_cm.getCfgValue("login", "remember");

    QByteArray user_des=QByteArray::fromBase64(user_base64.toUtf8());

    unsigned char Outdata[4*1024];
    int  Outlen;

    int ret=DesDec((unsigned char *)user_des.data(),user_des.length(),Outdata,&Outlen);
    if(ret==0)
    {
        QString user=QString::fromUtf8((const char *)Outdata,Outlen);
        ui->name->setText(user);
    }

    if(remember=="yes")
    {
        QByteArray pwd_des=QByteArray::fromBase64(pwd_base64.toUtf8());

        unsigned char Outdata[4*1024];
        int  Outlen;

        int ret=DesDec((unsigned char *)pwd_des.data(),pwd_des.length(),Outdata,&Outlen);
        if(ret==0)
        {
            QString pwd=QString::fromUtf8((const char *)Outdata,Outlen);
            ui->passwd->setText(pwd);
        }
    }

    QString ip=m_cm.getCfgValue("web_server", "ip");
    QString port=m_cm.getCfgValue("web_server", "port");

    ui->serverIP->setText(ip);
    ui->serverPORT->setText(port);

}

void login::on_registerbutton_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->reg_page);
    ui->registerName->setFocus();
}

void login::on_setButton_clicked()
{
    QString ip=ui->serverIP->text();
    QString port=ui->serverPORT->text();

    QRegExp reg(IP_REG);
    if(!reg.exactMatch(ip))
    {
        QMessageBox::warning(this,"警告","输入的服务器IP格式错误");
        return;
    }

    reg.setPattern(PORT_REG);
    if(!reg.exactMatch(port))
    {
        QMessageBox::warning(this,"警告","输入的服务器PORT格式错误");
        return;
    }

    m_cm.writeWebInfo(ip,port);
    ui->stackedWidget->setCurrentWidget(ui->login_page);
}

void login::on_registerConfirm_clicked()
{
    QString name = ui->registerName->text();
    QString nickName = ui->registerNickname->text();
    QString passwd = ui->registerPwd->text();
    QString pwd_confirm = ui->registerRepeatPwd->text();
    QString phone = ui->registerPhone->text();
    QString email = ui->registerEmail->text();
    QString address = ui->serverIP->text();
    QString port = ui->serverPORT->text();
    // 数据校验
    QRegExp regexp(USER_REG);
    if(!regexp.exactMatch(name))
    {
        QMessageBox::warning(this ,"警告","用户名格式不正确");
        //清除输入框中的内容
        ui->registerName->clear();
        //光标定位到这里
        ui->registerName->setFocus();
        return;
    }

     regexp.setPattern(USER_REG);
     if(!regexp.exactMatch(nickName))
     {
         QMessageBox::warning(this ,"警告","昵称格式不正确");
         //清除输入框中的内容
         ui->registerNickname->clear();
         //光标定位到这里
         ui->registerNickname->setFocus();
         return;
     }

    regexp.setPattern(PASSWD_REG);
    if(!regexp.exactMatch(passwd))
    {
        QMessageBox::warning(this ,"警告","密码格式不正确");
        //清除输入框中的内容
        ui->registerPwd->clear();
        //光标定位到这里
        ui->registerPwd->setFocus();

        return;
    }


    regexp.setPattern(PASSWD_REG);
    if(!regexp.exactMatch(pwd_confirm))
    {
        QMessageBox::warning(this ,"警告","确认密码格式不正确");
        //清除输入框中的内容
        ui->registerRepeatPwd->clear();
        //光标定位到这里
        ui->registerRepeatPwd->setFocus();

        return;
    }

    regexp.setPattern(PHONE_REG);
    if(!regexp.exactMatch(phone))
    {
        QMessageBox::warning(this ,"警告","手机号码格式不正确");
        //清除输入框中的内容
        ui->registerPhone->clear();
        //光标定位到这里
        ui->registerPhone->setFocus();

        return;
    }

    regexp.setPattern(EMAIL_REG);
    if(!regexp.exactMatch(email))
    {
        QMessageBox::warning(this ,"警告","邮箱格式不正确");
        //清除输入框中的内容
        ui->registerEmail->clear();
        //光标定位到这里
        ui->registerEmail->setFocus();

        return;
    }

    // 组织要发送的json字符串
    QByteArray data = getRegisterJson(name,nickName,passwd,phone,email);

    // 发送 http 请求协议给server - > post请求
    QNetworkAccessManager *manage=m_cm.getNetManager();

    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    request.setHeader(QNetworkRequest::ContentLengthHeader,data.size());

    QString url=QString("http://%1:%2/reg").arg(address).arg(port);
    request.setUrl(QUrl(url));

    QNetworkReply *reply=manage->post(request,data);
    connect(reply,&QNetworkReply::readyRead,[=]()
            {
                QByteArray data=reply->readAll();

                QString status=m_cm.getCode(data);
                if("002"==status)
                {
                    QMessageBox::information(this, "恭喜", "用户注册成功");
                    ui->name->setText(ui->registerName->text());
                    ui->passwd->setText(ui->registerPwd->text());

                    ui->registerName->clear();
                    ui->registerPwd->clear();
                    ui->registerRepeatPwd->clear();
                    ui->registerPhone->clear();
                    ui->registerEmail->clear();

                    ui->name->setFocus();
                    ui->stackedWidget->setCurrentWidget(ui->login_page);

                }
                else if("003" == status)
                {
                    QMessageBox::warning(this, "注册失败", QString("[%1]该用户已经存在!!!").arg(name));
                }
                else
                {
                    QMessageBox::warning(this, "注册失败", "注册失败！！！");
                }

                reply->deleteLater();
            });

}

void login::on_loginbutton_clicked()
{
    QString name = ui->name->text();
    QString passwd = ui->passwd->text();

    QString address=ui->serverIP->text();
    QString port=ui->serverPORT->text();
    // 数据校验
    QRegExp regexp(USER_REG);
    if(!regexp.exactMatch(name))
    {
        QMessageBox::warning(this ,"警告","用户名格式不正确");
        //清除输入框中的内容
        ui->name->clear();
        //光标定位到这里
        ui->name->setFocus();
    }

    regexp.setPattern(PASSWD_REG);
    if(!regexp.exactMatch(passwd))
    {
        QMessageBox::warning(this ,"警告","手机号码格式不正确");
        //清除输入框中的内容
        ui->passwd->clear();
        //光标定位到这里
        ui->passwd->setFocus();

        return;
    }

    // 组织要发送的json字符串
    QByteArray data = getLoginJson(name,passwd);

    // 发送 http 请求协议给server - > post请求
    QNetworkAccessManager *manage=m_cm.getNetManager();

    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    request.setHeader(QNetworkRequest::ContentLengthHeader,data.size());

    QString url=QString("http://%1:%2/login").arg(address).arg(port);
    request.setUrl(QUrl(url));

    QNetworkReply *reply=manage->post(request,data);
    connect(reply,&QNetworkReply::readyRead,[=]()
            {
                QByteArray data=reply->readAll();

                QStringList list=parseLoginJson(data);
                qDebug()<<list.at(0);
                if("000"==list.at(0))
                {
                    LoginInfoInstance *logininstance=LoginInfoInstance::getInstance();
                    logininstance->setLoginInfo(name,address,port,list.at(1));

                    m_cm.writeLoginInfo(name,passwd,ui->checkBox->isChecked());

                    mainWindow=new mainWidget;

                    mainWindow->setWindowFlag(Qt::FramelessWindowHint);
                    mainWindow->setWindowIcon(QIcon(":/images/logo.ico"));

                    this->hide();

                    mainWindow->showMainWindow();

                    connect(mainWindow,&mainWidget::changeUser,[=]()
                            {
                                this->show();
                                mainWindow->hide();

                                mainWindow->deleteLater();//不能立即释放mainWindow指针，会出现段错误

                            });
                }
                else
                {
                    QMessageBox::warning(this, "登陆失败", "登陆失败！！！！！");
                }

                reply->deleteLater();
            });

}
