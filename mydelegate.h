#ifndef MYDELEGATE_H
#define MYDELEGATE_H

#include <QObject>

class MyDelegate : public QObject
{
    Q_OBJECT
public:
    explicit MyDelegate(QObject *parent = nullptr);

signals:

};

#endif // MYDELEGATE_H
