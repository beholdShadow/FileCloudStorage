#ifndef LOGIN_H
#define LOGIN_H

#include <QWidget>
#include "./common/common.h"
#include "./common/des.h"
#include "./common/logininfoinstance.h"
#include "mainwidget.h"

namespace Ui {
class login;
}

class login : public QWidget
{
    Q_OBJECT

public:
    explicit login(QWidget *parent = nullptr);
    ~login();

    //注册信息组成JSON包
    QByteArray getRegisterJson(QString name, QString nick, QString passwd, QString phone, QString email);   
    //登陆信息组成JSON包
    QByteArray getLoginJson(QString name, QString passwd);
    // 解析服务器返回的json字符串
    QStringList parseLoginJson(QByteArray json);
    // 登录时加载配置文件
    void loadLoginConfig();

protected:
    void paintEvent(QPaintEvent *);
    void mouseMoveEvent(QMouseEvent *ev);
    void mousePressEvent(QMouseEvent *ev);

private slots:
    void on_registerbutton_clicked();

    void on_setButton_clicked();

    void on_registerConfirm_clicked();

    void on_loginbutton_clicked();

private:
    Ui::login *ui;
    Common m_cm;
    QString token;
    QPoint p;
    mainWidget *mainWindow;
};



#endif // LOGIN_H
