#include "repository.h"

Repository::Repository()
{
    m_queue_MaxSize=10;
}

void Repository::AddData(std::string& str)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    while(m_queue.size()>=m_queue_MaxSize)
    {
        m_queue_not_Full.wait(lock);
    }

    m_queue.push(str);

    m_queue_not_Empty.notify_all();

    lock.unlock();
}

std::string Repository::GetData()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    std::string str;

    while(m_queue.size()<=0)
    {
        m_queue_not_Empty.wait(lock);
    }

    str=m_queue.front();
    m_queue.pop();

    m_queue_not_Full.notify_all();

    lock.unlock();

    return str;

}
Repository::~Repository()
{

}

int Repository::getQueuesize()
{
    return m_queue.size();
}
