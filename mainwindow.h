#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QThread>
#include <QMessageBox>
#include <QFileDialog>
#include <QDebug>
#include <QCloseEvent>
#include <QGridLayout>
#include "fileprocessor.h"
#include "types.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}

QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btnBrowseInput_clicked();
    void on_btnBrowseOutput_clicked();
    void on_btnStart_clicked();
    void on_btnPause_clicked();
    void on_btnStop_clicked();

    void onStatusMessage(const QString& message);
    void onFileStarted(const QString &fileName);
    void onProgressUpdated(int percetage);
    void onFileFinished(const QString& fileName);
    void onAllWorkFinished();
    void onErrorOccured(const QString& error);
    void onTimerTicked();

public: signals:
    void pause();
    void stop();
    void resume();
    void start(const AppConfig& config);

protected:

    void closeEvent(QCloseEvent* event) override;

private:
    void setThread();
    void setoffGrid(QGridLayout* grid, bool isProcessing);
    void updateUIState(bool isProcessing);
    AppConfig fillConfig();
    bool checkInputs();

    Ui::MainWindow *ui;

    QThread* m_workerThread;
    QTimer* m_timer;

    bool m_isProcessingActive;

    FileProcessor* m_processor;
};
#endif // MAINWINDOW_H
