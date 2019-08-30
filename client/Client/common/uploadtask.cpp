#include "uploadtask.h"
#include <QFileInfo>
#include "uploadlayout.h"
#include "common/common.h"

UploadTask* UploadTask::instance=new UploadTask;
UploadTask::Garbo UploadTask::temp;

UploadTask * UploadTask::getInstance()
{
    return instance;
}

//追加上传文件到上传列表中
//参数：path 上传文件路径
//返回值：成功为0
//失败：
//  -1: 文件大于30m
//  -2：上传的文件是否已经在上传队列中
//  -3: 打开文件失败
//  -4: 获取布局失败
int UploadTask::appendUploadList(QString path)
{
    QFileInfo fileinfo(path);
    qint64 filesize=fileinfo.size();

//    cout<<"fileinfo.size()"<<fileinfo.size()<<" filesize"<<filesize;;

    if(filesize>30*1024*1024)
    {
        cout<<"选择上传的"<<fileinfo.fileName()<<"文件太大"<<endl;
        return -1;
    }

    for (int i=0;i<list.size();i++)
    {
        if(list.at(i)->fileName==fileinfo.fileName())
        {
            cout<<fileinfo.fileName()<<"文件已经存在传输列表"<<endl;
            return -2;
        }
    }

    Common cm;

    QFile *file=new QFile(path);
    if(!file->open(QIODevice::ReadOnly))
    {
        //如果打开文件失败，则删除 file，并使 file 指针为 0，然后返回
        cout << "file open error";
        delete file;
        file = nullptr;
        return -3;
    }

    DataProgress *dp=new DataProgress;
    dp->setFileName(fileinfo.fileName());

    UploadFileInfo* uploadfileinfo=new UploadFileInfo;
    uploadfileinfo->md5=cm.getFileMd5(path);
    uploadfileinfo->file=file;
    uploadfileinfo->path=path;
    uploadfileinfo->fileName=fileinfo.fileName();
    uploadfileinfo->isUpload=false;
    uploadfileinfo->dp=dp;
    uploadfileinfo->size=filesize;

    UploadLayout *uploadlayout=UploadLayout::getInstance();
    if(uploadlayout==nullptr)
    {
        cout<<"上传列表布局获取失败"<<endl;
        return -4;
    }

    QVBoxLayout *vlayout=(QVBoxLayout*)uploadlayout->getUploadLayout();
    vlayout->insertWidget(vlayout->count()-1,dp);

    cout<<fileinfo.fileName()<<"成功加入上传列表"<<endl;

    list.append(uploadfileinfo);

    return 0;
}

//判断上传队列释放为空
bool UploadTask::isEmpty()
{
    return list.isEmpty();
}

//是否有文件正在上传
bool UploadTask::isUpload()
{
    for (int i=0;i<list.size();i++)
    {
        if(list.at(i)->isUpload)
            return true;
    }
    return false;
}

//取出第0个上传任务，如果任务队列没有任务在上传，设置第0个任务上传
UploadFileInfo * UploadTask::takeTask()
{
    if(!list.at(0)->isUpload)
            list.at(0)->isUpload=true;

    return list.at(0);
}

//删除上传完成的任务
void UploadTask::dealUploadTask()
{
    for (int i=0;i<list.size();i++)
    {
        if(list.at(i)->isUpload)
        {
             UploadFileInfo *tmp=list.takeAt(i);
             UploadLayout *uploadlayout=UploadLayout::getInstance();
             QLayout* layout=uploadlayout->getUploadLayout();
             layout->removeWidget(tmp->dp);

             tmp->file->close();

             delete tmp->dp;
             delete tmp->file;
             delete tmp;

            return;
        }
    }
}

//清空上传列表
void UploadTask::clearList()
{
    int count=list.size();
    for (int i=0;i<count; ++i)
    {
        UploadFileInfo *info=list.takeFirst();

        delete  info;
    }
}
