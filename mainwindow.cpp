#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "appconfig.h"

#include <QMessageBox>
#include <QMenu>
#include <QVBoxLayout>
#include <QTime>
#include <QLocale>
#include <QFileInfo>
#include <QDir>
#include <QUrl>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("SETP Ver 2");

    // Nap config MOT LAN DUY NHAT o day (truoc day bi doc rai rac o 3 cho)
    QString configPath = "/Users/nguyenduchung/Documents/Study/2025.2/Thuc_Tap_2/Learning/Lession1/Project2_ver2/config.ini";
    AppConfig::instance().load(configPath);
    const auto &config = AppConfig::instance();

    // ----- Controllers -----
    m_sftpController = new SftpController(this);
    m_localFileController = new LocalFileController(this);

    ui->treeView->setModel(m_sftpController->fileModel());

    // treeview_Local - van dung QFileSystemModel truc tiep, ban than no da la
    // 1 Model dung chuan cua Qt roi nen khong can boc them lop nao khac
    m_localModel = new QFileSystemModel(this);
    m_localModel->setRootPath(config.localPath());
    m_localModel->setNameFilters(QStringList() << "*.mp4" << "*.webm" << "*.mov" << "*.mkv" << "*.avi");
    m_localModel->setNameFilterDisables(true);

    ui->treeView_Local->setModel(m_localModel);
    ui->treeView_Local->setRootIndex(m_localModel->index(config.localPath()));
    ui->treeView_Local->hideColumn(1);
    ui->treeView_Local->hideColumn(2);

    // Video
    QVideoWidget *videoWidget = new QVideoWidget(ui->groupBoxVideo);
    videoWidget->hide();
    QVBoxLayout *videoLayout = new QVBoxLayout(ui->groupBoxVideo);
    videoLayout->setContentsMargins(5, 5, 5, 5);
    videoLayout->addWidget(videoWidget);

    m_playerController = new PlayerController(videoWidget, this);

    // Thiet lap giao dien
    ui->pushButton_Play_Pause->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    ui->pushButton_Seek_Backward->setIcon(style()->standardIcon(QStyle::SP_MediaSkipBackward));
    ui->pushButton_Seek_Forward->setIcon(style()->standardIcon(QStyle::SP_MediaSkipForward));
    ui->btnConnect->setStyleSheet("color: white; background-color: green");

    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->treeView_Local->setContextMenuPolicy(Qt::CustomContextMenu);

    // ----- Ket noi Controller <-> View -----
    connect(ui->btnConnect, &QPushButton::clicked, this, &MainWindow::onBtnConnectClicked);

    connect(m_sftpController, &SftpController::connectionStateChanged, this, &MainWindow::handleConnectionStateChanged);
    connect(m_sftpController, &SftpController::connectionFailed, this, &MainWindow::handleConnectionFailed);
    connect(m_sftpController, &SftpController::videoReadyToPlay, this, &MainWindow::handleVideoReadyToPlay);

    connect(m_playerController, &PlayerController::error, this, &MainWindow::handlePlayerError);
    connect(m_playerController, &PlayerController::durationChangedSec, this, &MainWindow::handleDurationChangedSec);
    connect(m_playerController, &PlayerController::positionChangedSec, this, &MainWindow::handlePositionChangedSec);
    connect(m_playerController, &PlayerController::playStateChanged, this, &MainWindow::handlePlayStateChanged);
}

MainWindow::~MainWindow()
{
    // Delete tmp
    QDir tmpDir(AppConfig::instance().tmpDir());
    tmpDir.removeRecursively();
    delete ui;
}

// ---------------- Buttons ----------------

void MainWindow::onBtnConnectClicked()
{
    m_sftpController->toggleConnection();
}

void MainWindow::on_btnPlayStream_clicked()
{
    QUrl rtsp(AppConfig::instance().rtspLink());
    m_playerController->playUrl(rtsp);
}

void MainWindow::on_pushButton_Play_Pause_clicked()
{
    m_playerController->togglePlayPause();
}

void MainWindow::on_horizontalSlider_Duration_sliderMoved(int position)
{
    updateTimeLabels(position);
}

void MainWindow::on_pushButton_Seek_Forward_clicked()
{
    playNeighbourInLocalTree(1);
}

void MainWindow::on_pushButton_Seek_Backward_clicked()
{
    playNeighbourInLocalTree(-1);
}

// ---------------- Tree view remote ----------------

void MainWindow::on_treeView_doubleClicked(const QModelIndex &index)
{
    QString name = index.data().toString().trimmed();
    m_sftpController->openEntry(name);
}

void MainWindow::on_treeView_customContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = ui->treeView->indexAt(pos);
    QString fileName = index.data().toString().trimmed();

    QMenu menu;
    QAction *renameAction = menu.addAction("Rename");
    QAction *downloadAction = menu.addAction("Download");
    QAction *refreshAction = menu.addAction("Refresh");
    QAction *action = menu.exec(ui->treeView->viewport()->mapToGlobal(pos));

    if (action == renameAction) {
        promptRenameRemote(fileName);
    } else if (action == downloadAction) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Download window",
            "Do you really want to download this file to your local machine??",
            QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            m_sftpController->downloadToLocal(fileName);
        }
    } else if (action == refreshAction) {
        m_sftpController->refresh();
    }
}

// ---------------- Tree view local ----------------

void MainWindow::on_treeView_Local_clicked(const QModelIndex &index)
{
    if (m_localModel->isDir(index)) {
        ui->label_Video_Info->setText("This is Dir");
        return;
    }
    QString filePath = m_localModel->filePath(index);
    QFileInfo info(filePath);
    QString sizeText = QLocale::system().formattedDataSize(info.size());
    ui->label_Video_Info->setText(QString("%1  [%2]").arg(info.fileName()).arg(sizeText));
}

void MainWindow::on_treeView_Local_doubleClicked(const QModelIndex &index)
{
    QString name = index.data().toString().trimmed();
    QString localPath = m_localModel->filePath(index);
    setWindowTitle(name);
    m_playerController->play(localPath);
}

void MainWindow::on_treeView_Local_customContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = ui->treeView_Local->indexAt(pos);
    QString fileName = index.data().toString().trimmed();

    QMenu menu;
    QAction *renameAction = menu.addAction("Rename");
    QAction *deleteAction = menu.addAction("Delete");
    QAction *action = menu.exec(ui->treeView_Local->viewport()->mapToGlobal(pos));

    if (!action) return;

    if (action == renameAction) {
        promptRenameLocal(fileName);
    } else if (action == deleteAction) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Delete window", "Do you really want to Delete this file??",
            QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            m_localFileController->deleteFile(fileName);
        }
    }
}

// ---------------- Nhận signal từ Controller ----------------

void MainWindow::handleConnectionStateChanged(bool connected)
{
    ui->btnConnect->setText(connected ? "Disconnect" : "Connect");
}

void MainWindow::handleConnectionFailed(const QString &reason)
{
    QMessageBox::warning(this, "Error", reason);
}

void MainWindow::handleVideoReadyToPlay(const QString &localPath)
{
    m_playerController->play(localPath);
}

void MainWindow::handlePlayerError(const QString &message)
{
    QMessageBox::warning(this, "Error", message);
}

void MainWindow::handleDurationChangedSec(qint64 durationSec)
{
    m_totalDurationSec = durationSec;
    ui->horizontalSlider_Duration->setMaximum(durationSec);
}

void MainWindow::handlePositionChangedSec(qint64 positionSec)
{
    if (!ui->horizontalSlider_Duration->isSliderDown()) {
        ui->horizontalSlider_Duration->setValue(positionSec);
    }
    updateTimeLabels(positionSec);
}

void MainWindow::handlePlayStateChanged(bool isPaused)
{
    ui->pushButton_Play_Pause->setIcon(
        style()->standardIcon(isPaused ? QStyle::SP_MediaPlay : QStyle::SP_MediaPause));
}

// ---------------- Helpers ----------------

void MainWindow::updateTimeLabels(qint64 currentSec)
{
    if (currentSec == 0 && m_totalDurationSec == 0) return;

    QTime currentTime((currentSec / 3600) % 60, (currentSec / 60) % 60, currentSec % 60);
    QTime totalTime((m_totalDurationSec / 3600) % 60, (m_totalDurationSec / 60) % 60, m_totalDurationSec % 60);

    QString format = (m_totalDurationSec > 3600) ? "hh:mm:ss" : "mm:ss";
    ui->label_current_Time->setText(currentTime.toString(format));
    ui->label_Total_Time->setText(totalTime.toString(format));
}

void MainWindow::promptRenameRemote(const QString &oldName)
{
    Dialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) return; // chỉ gọi exec() 1 lần duy nhất

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Rename window", "Do you really want to change name file ??",
        QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    QString newName = dialog.NewName_Test();
    m_sftpController->renameRemote(oldName, newName);
}

void MainWindow::promptRenameLocal(const QString &oldName)
{
    Dialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) return; // chỉ gọi exec() 1 lần duy nhất

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Rename window", "Do you really want to change name file ??",
        QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    QString newName = dialog.NewName_Test();
    m_localFileController->renameFile(oldName, newName);
}

void MainWindow::playNeighbourInLocalTree(int rowOffset)
{
    QModelIndex current = ui->treeView_Local->currentIndex();
    if (!current.isValid()) return;

    QModelIndex neighbour = current.siblingAtRow(current.row() + rowOffset);
    if (!neighbour.isValid()) return;

    ui->treeView_Local->setCurrentIndex(neighbour);
    QString name = neighbour.data().toString().trimmed();
    QString localPath = m_localModel->filePath(neighbour);
    setWindowTitle(name);
    m_playerController->play(localPath);
}
