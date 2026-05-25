#ifndef TYPES_H
#define TYPES_H

#include <QString>
#include <QByteArray>

enum class Action {
    Overwrite,
    Rename
};

struct AppConfig {
    QString fileMask;
    QString outputDir;
    QString inputDir;

    bool deleteInputFiles = false;
    bool runByTimer = false;

    int timerInterval = 1000;

    QByteArray hex;

    Action action = Action::Rename;
};

#endif // TYPES_H
