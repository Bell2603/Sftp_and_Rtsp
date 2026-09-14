#ifndef PLAYERCONTROLLER_H
#define PLAYERCONTROLLER_H

#include <QObject>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QUrl>

// Controller: quản lý QMediaPlayer + QVideoWidget.
// MainWindow không còn tự giữ biến IS_Pause, mDuration nữa - toàn bộ
// trạng thái play/pause/duration nằm ở đây, MainWindow chỉ lắng nghe signal.
class PlayerController : public QObject
{
    Q_OBJECT
public:
    explicit PlayerController(QVideoWidget *videoWidget, QObject *parent = nullptr);

    // Phát file local (kiểm tra file tồn tại trước khi phát)
    void play(const QString &localPath);

    // Phát trực tiếp URL (dùng cho RTSP stream, không kiểm tra QFile::exists)
    void playUrl(const QUrl &url);

    void togglePlayPause();

    bool isPaused() const { return m_isPaused; }
    qint64 totalDurationSec() const { return m_durationSec; }

signals:
    void error(const QString &message);
    void durationChangedSec(qint64 durationSec);
    void positionChangedSec(qint64 positionSec);
    void playStateChanged(bool isPaused);

private slots:
    void handleDurationChanged(qint64 duration);
    void handlePositionChanged(qint64 position);

private:
    void startPlayback();

    QMediaPlayer *m_player;
    QVideoWidget *m_videoWidget;
    bool m_isPaused = true;
    qint64 m_durationSec = 0;
};

#endif // PLAYERCONTROLLER_H
