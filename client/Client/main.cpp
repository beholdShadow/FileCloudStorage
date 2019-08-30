#include "login.h"
#include <QApplication>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    login w1;
    w1.show();
    return a.exec();
}
