#ifndef FILEPROCESSOR_H
#define FILEPROCESSOR_H

#include <QObject>
#include <QMutex>
#include <QWaitCondition>
#include <atomic>
#include "types.h"

class FileProcessor : public QObject {
    Q_OBJECT
public:
    explicit FileProcessor(QObject *parent = nullptr);

    ~FileProcessor();

signals:
    void statusMessege(const QString& message);

    void fileStarted(const QString &filename);
    void fileFinished(const QString& filename);

    void progressUpdated(int percent);

    void allWorkFinished();

    void errorOccured(const QString& error);

public slots:
    void start(const AppConfig& config);
    void pause();
    void resume();
    void cancel();

private:

    void runProcessing();
    bool proccessingSingleFile(const QString &inputPath, const AppConfig& config);
    QString generateOutPath(const QString& inputPath, const AppConfig& config);

    std::atomic<bool> m_isPaused{false};
    std::atomic<bool> m_isCancelled{false};

    AppConfig m_config;
};

#endif // FILEPROCESSOR_H
