#ifndef TITLEWG_H
#define TITLEWG_H

#include <QWidget>

namespace Ui {
class Titlewg;
}

class Titlewg : public QWidget
{
    Q_OBJECT

public:
    explicit Titlewg(QWidget *parent = nullptr);
    ~Titlewg();
signals:
    void closeWindow();
    void showSet();
private slots:
    void on_set_clicked();

    void on_min_clicked();

    void on_close_clicked();

private:
    Ui::Titlewg *ui;
    QWidget *m_parent;
};

#endif // TITLEWG_H
