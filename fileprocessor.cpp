#include "fileprocessor.h"
#include <QDebug>
#include <QDir>
#include <QThread>
#include <QFileInfo>
#include <qabstracteventdispatcher.h>

FileProcessor::FileProcessor(QObject *parent) : QObject{parent} {
    m_isPaused = false;
    m_isCancelled = false;
}

FileProcessor::~FileProcessor() {
    cancel();
}

void FileProcessor::pause() {
    m_isPaused = true;

}

void FileProcessor::resume() {

        m_isPaused = false;
        m_pauseCondition.wakeAll();


}

void FileProcessor::cancel() {

    m_isCancelled = true;

    m_isPaused = false;
    m_pauseCondition.wakeAll();
}

void FileProcessor::start(const AppConfig& config) {

    m_config = config;
    m_isPaused = false;
    m_isCancelled = false;

    runProcessing();
}

void FileProcessor::runProcessing() {

    emit statusMessege("Scanning directory");

    QDir inputDir(m_config.inputDir);

    if(!inputDir.exists()) {

        emit errorOccured("Input directory doesnt exist");
        emit allWorkFinished();
        return;
    }

    QStringList filters;
    if(!m_config.fileMask.isEmpty()) {
        filters = m_config.fileMask.split(';', Qt::SkipEmptyParts);
    }
    else {
        filters << "*.*";
    }

    QFileInfoList fileList = inputDir.entryInfoList(filters, QDir::Files | QDir::NoSymLinks);

    if(fileList.isEmpty()) {
        emit errorOccured("No files found in input directory matching the specified mask.");
        emit allWorkFinished();
        return;
    }

    int processedCount = 0;

    for(const QFileInfo &fileInfo : fileList) {

        if(m_isCancelled)
            break;

        emit fileStarted(fileInfo.fileName());

            emit statusMessege(QString("Start processing file №%1").arg(processedCount + 1));

        bool isSuccess = proccessingSingleFile(fileInfo.absoluteFilePath(), m_config);

        if(isSuccess) {
            processedCount++;

            emit fileFinished(fileInfo.fileName());

            if(m_config.deleteInputFiles) {
                QFile::remove(fileInfo.absoluteFilePath());
            }
        }

    }
        emit statusMessege(QString("Successfully processed files - %1").arg(processedCount));

    emit allWorkFinished();
}

bool FileProcessor::proccessingSingleFile(const QString& inputPath, const AppConfig& config) {

    QFile inFile(inputPath);

    if(!inFile.open(QIODevice::ReadOnly)) {
        emit errorOccured("File couldnt be opened for reading: " + inFile.errorString());
        return false;
    }

    QString outPath = generateOutPath(inputPath, config);
    QFile outFile(outPath);

    if(!outFile.open(QIODevice::WriteOnly)) {

        emit errorOccured("Input file couldnt be created");
        inFile.close();
        return false;
    }

    qint64 fileSize = inFile.size();
    qint64 bytesProcessed = 0;
    qint64 buffer_size = 1024 * 1024; // 1 Mb

    QByteArray buffer;
    buffer.resize(buffer_size);

    const QByteArray& key = config.hex;
    int keyLength = key.length();
    qint64 globalByteIndex = 0;

    while(!inFile.atEnd()) {

        if (QThread::currentThread()->eventDispatcher())
            QThread::currentThread()->eventDispatcher()->processEvents(QEventLoop::AllEvents);


        if(m_isPaused) {
            m_pauseMutex.lock();

            while(m_isPaused && !m_isCancelled) {
                m_pauseCondition.wait(&m_pauseMutex);
            }

            m_pauseMutex.unlock();

        }

        if(m_isCancelled) {
            inFile.close();
            outFile.close();
            outFile.remove();
            return false;
        }

        qint64 bytesRead = inFile.read(buffer.data(), buffer_size);

        if(bytesRead <= 0)
            break;

        for(qint64 i = 0; i < bytesRead; i++) {

            char keyByte = (keyLength > 0) ? key.at(globalByteIndex % keyLength) : 0;

            buffer.data()[i] ^= keyByte;

            globalByteIndex++;

        }

        outFile.write(buffer.constData(), bytesRead);
        bytesProcessed += bytesRead;

        if(fileSize > 0) {
            int progress = static_cast<int>((bytesProcessed * 100) / fileSize);
            emit progressUpdated(progress);
        }


    }


    inFile.close();
    outFile.close();

    return true;

}

QString FileProcessor::generateOutPath(const QString& input, const AppConfig& config) {

    QFileInfo fileInfo(input);
    QString baseName = fileInfo.completeBaseName();
    QString suffix = fileInfo.suffix();

    QString targetPath = QDir(config.outputDir).filePath(fileInfo.fileName());

    if(!QFile::exists(targetPath) || config.action == Action::Overwrite)
        return targetPath;

    int counter = 1;
    QString newPath;

    do {
        QString newName = QString("%1_%2.%3").arg(baseName).arg(counter).arg(suffix);
        newPath = QDir(config.outputDir).filePath(newName);
        counter++;
    } while(QFile::exists(newPath));

    return newPath;
}

