#pragma once

#include <QtWidgets/QWidget>
#include <QMediaPlayer>
#include <QMessagebox>
#include <QTime>
#include <QAudioOutput>
#include <QFile>
#include <QMouseEvent>
#include "ESParser.h"
#include "ui_MainWindow.h"

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindowW ui;

    
    QMediaPlayer* corePlayer;
    QAudioOutput* audioOutput;
    ESParser* esParser;
    QProcess* ffmpegTemopChanger;
    QString oriFileName;
    QVector<int64_t> esPartsPosition;
    int32_t lastTimezoneHighlighted;
    QLabel* playProgressPrompt;
    bool isProgressSliderPressed;

    QPoint windowDiffPos;


    void playerLoadFile(QString fileName);
    void playerReloadFile();
    void playerSSControl();
    void changeProgress();
    void progressbarChangeVal();
    void refreshDuration();
    void refreshProgress();
    void onMediaStatusChanged();
    void onParserStatusChanged(ParserStatus status);
    void onParserResultReceived(QVector<int64_t> esResult);
    void onPartBtnClicked();
    void onSpeedCtrlChanged();
    void startTempoChanging(QString newTempo);
    void onTempoChangeFinished();
    void clearTimezoneTable();
    void paintParts(QVector<PartType> partsType);

    void setWindowStaytopStatus();
    void mousePressEvent(QMouseEvent* event)  override;
    void mouseMoveEvent(QMouseEvent* event) override;
};
