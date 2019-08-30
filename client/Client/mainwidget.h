#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include "common/common.h"

namespace Ui {
class mainWidget;
}

class mainWidget : public QWidget
{
    Q_OBJECT

public:
    explicit mainWidget(QWidget *parent = nullptr);
    ~mainWidget();
    // 显示主窗口
    void showMainWindow();
    // 处理信号
    void managerSignals();
    // 重新登陆
    void loginAgain();

signals:
    // 切换用户按钮信号
    void changeUser();

protected:
    void paintEvent(QPaintEvent *);
private:
    Ui::mainWidget *ui;

    Common m_cm;
};

#endif // MAINWIDGET_H
