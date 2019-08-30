#ifndef REPOSITORY_H
#define REPOSITORY_H

#include <iostream>
#include <condition_variable>
#include <QList>
#include "uploadtask.h"

class Repository
{
public:
    Repository();

    void AddData(std::string& str);

    std::string GetData();

    ~Repository();

    int getQueuesize();

private:
    std::condition_variable m_queue_not_Empty;
    std::condition_variable m_queue_not_Full;

    std::mutex m_mutex;
    int m_queue_MaxSize;

};

#endif // REPOSITORY_H
