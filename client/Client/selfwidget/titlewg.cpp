#include "titlewg.h"
#include "ui_titlewg.h"

Titlewg::Titlewg(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Titlewg)
{
    ui->setupUi(this);

    m_parent=parent;
}

Titlewg::~Titlewg()
{
    delete ui;
}

void Titlewg::on_set_clicked()
{
    emit showSet();
}

void Titlewg::on_min_clicked()
{
    m_parent->showMinimized();
}

void Titlewg::on_close_clicked()
{
    emit closeWindow();
}
