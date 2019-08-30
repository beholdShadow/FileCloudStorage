#ifndef RANKLIST_H
#define RANKLIST_H

#include <QWidget>
#include "common/common.h"

namespace Ui {
class ranklist;
}
// 文件信息
struct RankFileInfo
{
    QString filename;   // 文件名字
    int pv;             // 下载量
};
class ranklist : public QWidget
{
    Q_OBJECT

public:
    explicit ranklist(QWidget *parent = nullptr);
    ~ranklist();
    // 设置TableWidget表头和一些属性
    void initTableWidget();

    // 清空文件列表
    void clearshareFileList();

    // ==========>显示共享文件列表<==============
    void refreshFiles();                                // 显示共享的文件列表
    QByteArray setFilesListJson(int start, int count);  // 设置json包
    void getUserFilesList();                            // 获取共享文件列表
    void getFileJsonInfo(QByteArray data);              // 解析文件列表json信息，存放在文件列表中

    // 更新排行榜列表
    void refreshList();

private:

    Common m_cm;
    QNetworkAccessManager *m_manager;

    int  m_start;                      // 文件位置起点
    int  m_count;                      // 每次请求文件个数
    long m_userFilesCount;             // 用户文件数目
    QList<RankFileInfo *> m_list;   // 文件列表

    Ui::ranklist *ui;
};

#endif // RANKLIST_H
