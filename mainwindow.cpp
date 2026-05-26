#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("XOR Utility");

    QRegularExpression hexRegex("^[0-9A-Fa-f]{16}$");
    QRegularExpressionValidator *validator = new QRegularExpressionValidator(hexRegex, this);
    ui->txtHexKey->setValidator(validator);

    m_timer = new QTimer(this);

    setThread();

}

MainWindow::~MainWindow()
{
    if(m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait();
    }

    delete ui;
}

void MainWindow::setThread() {

    m_workerThread = new QThread(this);
    m_processor = new FileProcessor();

    m_processor->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::finished, m_processor, &QObject::deleteLater);

    connect(m_timer, &QTimer::timeout, this, &MainWindow::onTimerTicked);

    connect(m_processor, &FileProcessor::statusMessege, this, &MainWindow::onStatusMessage);
    connect(m_processor, &FileProcessor::fileStarted, this, &MainWindow::onFileStarted);
    connect(m_processor, &FileProcessor::progressUpdated, this, &MainWindow::onProgressUpdated);
    connect(m_processor, &FileProcessor::fileFinished, this, &MainWindow::onFileFinished);
    connect(m_processor, &FileProcessor::allWorkFinished, this, &MainWindow::onAllWorkFinished);
    connect(m_processor, &FileProcessor::errorOccured, this, &MainWindow::onErrorOccured);

    connect(this, &MainWindow::pause, m_processor, &FileProcessor::pause, Qt::QueuedConnection);
    connect(this, &MainWindow::stop, m_processor, &FileProcessor::cancel, Qt::QueuedConnection);
    connect(this, &MainWindow::resume, m_processor, &FileProcessor::resume, Qt::QueuedConnection);
    connect(this, &MainWindow::start, m_processor, &FileProcessor::start, Qt::QueuedConnection);

    m_workerThread->start();
}

bool MainWindow::checkInputs() {

    if(ui->txtInputDir->text().isEmpty()) {
        QMessageBox::warning(this, "Error", "Specify the search directory for the input files!");
        return false;
    }
    if(ui->txtOutputDir->text().isEmpty()) {
        QMessageBox::warning(this, "Error", "Specify the search directory for the output file!");
        return false;
    }
    if(ui->txtHexKey->text().length() < 16) {
        QMessageBox::warning(this, "Error", "The HEX key must consist of 16 characters");
        return false;
    }

    return true;
}

AppConfig MainWindow::fillConfig() {
    AppConfig config;

    config.inputDir = ui->txtInputDir->text();
    config.outputDir = ui->txtOutputDir->text();
    config.fileMask = ui->txtFileMask->text();
    config.hex = QByteArray::fromHex(ui->txtHexKey->text().toUtf8());
    config.deleteInputFiles = ui->chkDeleteInput->isChecked();
    config.action = (ui->radOverwrite->isChecked() == 1) ? Action::Overwrite : Action::Rename;
    config.runByTimer = ui->chkTimer->isChecked();
    config.timerInterval = ui->spnSingleRun->value() * 1000;

    return config;
}

void MainWindow::updateUIState(bool isProcessing) {

    m_isProcessingActive = isProcessing;

    ui->btnBrowseInput->setEnabled(!isProcessing);
    ui->btnBrowseOutput->setEnabled(!isProcessing);
    ui->txtFileMask->setEnabled(!isProcessing);
    ui->txtHexKey->setEnabled(!isProcessing);
    ui->chkDeleteInput->setEnabled(!isProcessing);
    ui->radOverwrite->setEnabled(!isProcessing);
    ui->chkTimer->setEnabled(!isProcessing);
    ui->spnSingleRun->setEnabled(!isProcessing);
    ui->radRename->setEnabled(!isProcessing);


    ui->btnStart->setEnabled(!isProcessing  && !m_timer->isActive());
    ui->btnPause->setEnabled(isProcessing || m_timer->isActive());
    ui->btnStop->setEnabled(isProcessing || m_timer->isActive());

    if(!isProcessing)
        ui->btnPause->setText("Pause");
}

void MainWindow::on_btnBrowseInput_clicked()
{

    QString dir = QFileDialog::getExistingDirectory(this, "Select input data files directory");

    if(!dir.isEmpty())
        ui->txtInputDir->setText(dir);
}


void MainWindow::on_btnBrowseOutput_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select output data file");

    if(!dir.isEmpty())
        ui->txtOutputDir->setText(dir);
}


void MainWindow::on_btnStart_clicked()
{
    if(!checkInputs())
        return;

    m_isStopProcessing = false;

    AppConfig config = fillConfig();

    ui->txtLog->clear();

    if(config.runByTimer) {
        ui->txtLog->append("Timer mode has been activated");
        m_timer->setInterval(config.timerInterval);

    }

    ui->progressBar->setValue(0);

    updateUIState(true);

    emit start(config);

}

void MainWindow::onTimerTicked() {

    m_timer->stop();

    updateUIState(true);

    AppConfig config = fillConfig();

    emit start(config);

}

void MainWindow::on_btnPause_clicked()
{
    if(ui->btnPause->text() == "Pause") {
        emit pause();
        ui->btnPause->setText("Resume");

        if(m_timer->isActive() && !m_isProcessingActive) {
            m_timer->stop();
        }


        ui->txtLog->append("Pause");
    }
    else {
        emit resume();
        ui->btnPause->setText("Pause");
        if(!m_isProcessingActive) {
            m_timer->start();
        }
        ui->txtLog->append("Resume");
    }
}


void MainWindow::on_btnStop_clicked()
{
    m_isStopProcessing = true;

    if(ui->btnPause->text() == "Resume") {
        ui->btnPause->setText("Pause");
        emit resume();
    }

    if(m_timer->isActive()) {
        m_timer->stop();
    }

    ui->txtLog->append("");
    ui->txtLog->append("Stop processing");

    ui->progressBar->setValue(0);
    updateUIState(false);

    emit stop();
}

void MainWindow::onStatusMessage(const QString &message) {

    ui->txtLog->append(message);

}

void MainWindow::onFileStarted(const QString& fileName) {
    ui->progressBar->setValue(0);
}

void MainWindow::onProgressUpdated(int percentage) {
    ui->progressBar->setValue(percentage);
}

void MainWindow::onFileFinished(const QString &filname) {

    ui->progressBar->setValue(100);

}

void MainWindow::onAllWorkFinished() {
    ui->progressBar->setValue(0);

    if(ui->chkTimer->isChecked() && !m_isStopProcessing) {
        ui->txtLog->append("");
        ui->txtLog->append("Waiting for next timer tick");
        m_timer->start();
    }

    updateUIState(false);

}

void MainWindow::onErrorOccured(const QString& error) {

    ui->txtLog->append(QString("<font color = 'red'> Error: %1 </font>").arg(error));

}

void MainWindow::closeEvent(QCloseEvent* event) {
    if(m_isProcessingActive) {
        QMessageBox::StandardButton resBtn = QMessageBox::question(this, "Warning", "Files processing is active. Are u sure you want to interrupt and exit?", QMessageBox::No | QMessageBox::Yes, QMessageBox::No);

        if(resBtn != QMessageBox::Yes) {
            event->ignore();
            return;
        }

        m_timer->stop();

        m_processor->cancel();

        if(m_workerThread) {
            m_workerThread->quit();
            m_workerThread->wait();
        }

    }

    event->accept();
}


