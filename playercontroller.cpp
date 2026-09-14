#include "playercontroller.h"

#include <QFile>

PlayerController::PlayerController(QVideoWidget *videoWidget, QObject *parent)
    : QObject(parent)
    , m_videoWidget(videoWidget)
{
    m_player = new QMediaPlayer(this);
    m_player->setVideoOutput(m_videoWidget);

    connect(m_player, &QMediaPlayer::durationChanged, this, &PlayerController::handleDurationChanged);
    connect(m_player, &QMediaPlayer::positionChanged, this, &PlayerController::handlePositionChanged);
}

void PlayerController::play(const QString &localPath)
{
    if (!QFile::exists(localPath)) {
        emit error("File not exist " + localPath);
        return;
    }

    if (!m_player->isAvailable()) {
        emit error("Video can't play");
    }

    m_player->stop();
    m_player->setSource(QUrl::fromLocalFile(localPath));
    startPlayback();
}

void PlayerController::playUrl(const QUrl &url)
{
    m_player->stop();
    m_player->setSource(url);

    if (m_player->error() != QMediaPlayer::NoError) {
        emit error("Video can't play");
    }

    startPlayback();
}

void PlayerController::startPlayback()
{
    m_videoWidget->show();
    m_player->play();

    // reset lai trang thai cua nut Play_Pause
    m_isPaused = false;
    emit playStateChanged(m_isPaused);
}

void PlayerController::togglePlayPause()
{
    if (m_isPaused) {
        m_isPaused = false;
        m_player->play();
    } else {
        m_isPaused = true;
        m_player->pause();
    }
    emit playStateChanged(m_isPaused);
}

void PlayerController::handleDurationChanged(qint64 duration)
{
    m_durationSec = duration / 1000; // Doi sang don vi giay
    emit durationChangedSec(m_durationSec);
}

void PlayerController::handlePositionChanged(qint64 position)
{
    emit positionChangedSec(position / 1000);
}
