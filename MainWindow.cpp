#include "MainWindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    lastTimezoneHighlighted = -1;
    isProgressSliderPressed = false;
    ffmpegTemopChanger = new QProcess;

    audioOutput = new QAudioOutput;
    audioOutput->setVolume(100);

    corePlayer = new QMediaPlayer;
    corePlayer->setAudioOutput(audioOutput);
    connect(corePlayer, &QMediaPlayer::durationChanged, this, &MainWindow::refreshDuration);
    connect(corePlayer, &QMediaPlayer::positionChanged, this, &MainWindow::refreshProgress);
    connect(corePlayer, &QMediaPlayer::mediaStatusChanged, this, &MainWindow::onMediaStatusChanged);

    esParser = new ESParser;
    connect(esParser, &ESParser::parserStatusChanged, this, &MainWindow::onParserStatusChanged);
    connect(esParser, &ESParser::ESResult, this, &MainWindow::onParserResultReceived);

    ui.setupUi(this);
    //ui.ctrl_speed->hide();
    this->setWindowTitle("ESAudioPlayer");

    connect(ui.ctrl_ss, &QPushButton::clicked, this, &MainWindow::playerSSControl);
    connect(ui.playProgress, &QSlider::sliderPressed, this, [=] {isProgressSliderPressed = true; });
    connect(ui.playProgress, &QSlider::valueChanged, this, &MainWindow::progressbarChangeVal);
    connect(ui.playProgress, &QSlider::sliderReleased, this, &MainWindow::changeProgress);
    connect(ui.ctrl_back, &QPushButton::clicked, this, [=] {corePlayer->setPosition(corePlayer->position() - 5000); });
    connect(ui.ctrl_forward, &QPushButton::clicked, this, [=] {corePlayer->setPosition(corePlayer->position() + 5000); });
    //connect(ui.ctrl_speed, &QComboBox::currentIndexChanged, this, &MainWindow::onSpeedCtrlChanged);

    connect(ui.wctrl_close, &QPushButton::clicked, this, [=] {this->close(); });
    connect(ui.wctrl_minisize, &QPushButton::clicked, this, [=] {this->showMinimized(); });
    connect(ui.wctrl_staytop, &QPushButton::clicked, this, &MainWindow::setWindowStaytopStatus);

    playProgressPrompt = new QLabel(this);
    playProgressPrompt->hide();
    playProgressPrompt->setStyleSheet(R"(
        font: 500 10pt "思源黑体 CN VF Medium";
        color: rgb(0, 0, 0);
        background-color: hsl(210, 5%, 100%);
        border-style: solid;
	    border-width: 2px;
	    border-radius: 2px;
	    border-color: hsl(210, 100%, 40%);
)");
    playProgressPrompt->setAlignment(Qt::AlignCenter);
    playProgressPrompt->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred);
    playProgressPrompt->setWindowFlag(Qt::FramelessWindowHint);

    this->setWindowFlags(Qt::FramelessWindowHint);

    if (qApp->arguments().size() >= 2)
        playerLoadFile(qApp->arguments().at(1));
    else
    {
        ui.playtime->setText("NO-FILE");
        ui.title->setText("请从要打开的文件引导启动");
        ui.statusbar->setText("无文件");
        ui.ctrl_ss->setDisabled(true);
    }
}

MainWindow::~MainWindow()
{}

void MainWindow::playerLoadFile(QString fileName)
{
    QFile file;
    file.setFileName(QCoreApplication::applicationDirPath() + "/current_media");
    file.remove();
    file.setFileName(fileName);
    file.copy(QCoreApplication::applicationDirPath() + "/current_media");
    corePlayer->setSource(QUrl::fromLocalFile(QCoreApplication::applicationDirPath() + "/current_media"));
    oriFileName = fileName;
    if(oriFileName.indexOf("\\")!=-1)
        ui.title->setText(oriFileName.split("\\").constLast());
    else
        ui.title->setText(oriFileName.split("/").constLast());
    if(ui.title->text().lastIndexOf(".")>0)
        ui.title->setText(ui.title->text().left(ui.title->text().lastIndexOf(".")));
    esParser->startParsing(QCoreApplication::applicationDirPath() + "/current_media");
}

//void MainWindow::playerReloadFile()
//{
//    corePlayer->setSource(QUrl::fromLocalFile(QCoreApplication::applicationDirPath() + "/current_media"));
//    if (abs(ui.ctrl_speed->currentIndex() - 3) > 2)
//    {
//        QPushButton* nullBtn = new QPushButton;
//        nullBtn->setText("过度变速情况下 ES 被禁用");
//        nullBtn->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
//        nullBtn->setEnabled(false);
//        ui.HL_timezone->addWidget(nullBtn);
//        return;
//    }
//    else
//    {
//        esParser->startParsing(QCoreApplication::applicationDirPath() + "/current_media");
//    }
//    lastTimezoneHighlighted = -1;
//}

void MainWindow::playerSSControl()
{
    if (corePlayer->isPlaying())
    {
        corePlayer->pause();
        ui.ctrl_ss->setIcon(QIcon(":/Res/play.svg"));
    }
    else
    {
        corePlayer->play();
        ui.ctrl_ss->setIcon(QIcon(":/Res/pause.svg"));
    }
}

void MainWindow::changeProgress()
{
    playProgressPrompt->hide();
    corePlayer->setPosition(ui.playProgress->value());
    isProgressSliderPressed = false;
}

void MainWindow::progressbarChangeVal()
{
    if (!isProgressSliderPressed)
        return;
    playProgressPrompt->show();
    if (abs(ui.playProgress->value() - corePlayer->position()) < 60000)
        if (ui.playProgress->value() - corePlayer->position() > 0)
            playProgressPrompt->setText(QString("+%1s").arg(QString::number((ui.playProgress->value() - corePlayer->position()) / 1000), 2, QChar('0')));
        else
            playProgressPrompt->setText(QString("%1s").arg(QString::number((ui.playProgress->value() - corePlayer->position()) / 1000), 2, QChar('0')));
    else
        playProgressPrompt->setText(QTime(0, 0, 0).addMSecs(ui.playProgress->value()).toString(QString::fromLatin1("mm:ss")));
    
    int32_t YPos = ui.playProgress->y() + ui.widget_timectrl->y() - 26;
    int32_t XPos = ui.playProgress->width()
        * ((double)ui.playProgress->value() / ui.playProgress->maximum())
        + ui.playProgress->x() - 22;
    if (XPos <= 2)
        XPos = 2;
    if (XPos >= ui.widget_timectrl->width() - 47)
        XPos = ui.widget_timectrl->width() - 47;
    playProgressPrompt->setGeometry(XPos, YPos, 45, 22);
}

void MainWindow::refreshDuration()
{
    ui.playProgress->setMaximum(corePlayer->duration());
}

void MainWindow::refreshProgress()
{
    if (!isProgressSliderPressed) 
    {
        ui.playProgress->setValue(corePlayer->position());
    }
    ui.playtime->setText(QTime(0, 0, 0).addMSecs(corePlayer->position()).toString(QString::fromLatin1("mm:ss")));
    if (esPartsPosition.count() > 0)
    {
        for (int32_t i = 0; i < esPartsPosition.count(); i++)
        {
            if (corePlayer->position() < esPartsPosition.at(i))
            {
                if(lastTimezoneHighlighted!=-1&& i!=lastTimezoneHighlighted)
                {
                    ui.HL_timezone->itemAt(lastTimezoneHighlighted)->widget()->setProperty("isCurrent", false);
                    ui.HL_timezone->itemAt(lastTimezoneHighlighted)->widget()->setStyleSheet(ui.HL_timezone->itemAt(lastTimezoneHighlighted)->widget()->styleSheet());
                }
                ui.HL_timezone->itemAt(i)->widget()->setProperty("isCurrent", true);
                ui.HL_timezone->itemAt(i)->widget()->setStyleSheet(ui.HL_timezone->itemAt(i)->widget()->styleSheet());
                lastTimezoneHighlighted = i;
                break;
            }
                
        }
    }
}

void MainWindow::onMediaStatusChanged()
{
    QMediaPlayer::MediaStatus status = corePlayer->mediaStatus();
    if(status == QMediaPlayer::MediaStatus::NoMedia)
        ui.statusbar->setText("未加载文件");
    if (status == QMediaPlayer::MediaStatus::InvalidMedia)
        ui.statusbar->setText("无效文件");
    if (status == QMediaPlayer::MediaStatus::LoadingMedia)
        ui.statusbar->setText("文件加载中...");
    if (status == QMediaPlayer::MediaStatus::EndOfMedia)
        ui.statusbar->setText("文件播放完毕");
    if (status == QMediaPlayer::MediaStatus::LoadedMedia)
        ui.statusbar->setText("文件准备就绪");
    if (!corePlayer->isPlaying())
        ui.ctrl_ss->setIcon(QIcon(":/Res/play.svg"));
    else
        ui.ctrl_ss->setIcon(QIcon(":/Res/pause.svg"));
}

void MainWindow::onParserStatusChanged(ParserStatus status)
{
    if (status == spectral_analysing)
    {
        ui.statusbar->setText("频谱分析进行中...");
    }
    if (status == silence_detect)
    {
        ui.statusbar->setText("停顿检测进行中...");
    }
    if (status == feature_analysing)
    {
        ui.statusbar->setText("特征分析进行中...");
    }
    if (status == parser_finished)
    {
        ui.statusbar->setText("音频分析完成");
    }
    if (status == parser_onerror)
    {
        ui.statusbar->setText("无法进行音频分析");
    }
}

void MainWindow::onParserResultReceived(QVector<int64_t> esResult)
{
    clearTimezoneTable();

    if (esResult.count() <= 5 || esResult.count() >= 30)
    {
        QPushButton *nullBtn = new QPushButton;
        nullBtn->setText("无可用切分");
        nullBtn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        nullBtn->setEnabled(false);
        ui.HL_timezone->addWidget(nullBtn);
        return;
    }
    int32_t partIndex = 0;
    int64_t lastPosition = 0;
    QVector<int32_t> parts;
    esResult.append(corePlayer->duration());

    for (auto current : esResult)
    {
        QPushButton* partBtn = new QPushButton;
        partBtn->setText(QString::number(partIndex));
        partBtn->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        ui.HL_timezone->addWidget(partBtn);
        ui.HL_timezone->setStretchFactor(partBtn, current-lastPosition);
        parts.append(current - lastPosition);
        if (partIndex == 0)
            partBtn->setProperty("isFirst", true);
        else
            partBtn->setProperty("isFirst", false);
        partBtn->setProperty("position", lastPosition);
        //partBtn->setStyleSheet();
        //QMessageBox::information(NULL, "调试信息", QString::number(current - lastPosition));
        connect(partBtn, &QPushButton::clicked, this, &MainWindow::onPartBtnClicked);
        partIndex += 1;
        lastPosition = current;
    }
    esPartsPosition = esResult;

    QVector<PartType> markResult = esParser->markParts(parts);
    if (markResult.size() == parts.size())
        paintParts(markResult);
}

void MainWindow::onPartBtnClicked()
{
    QPushButton* senderBtn = qobject_cast<QPushButton*>(sender());
    corePlayer->setPosition(senderBtn->property("position").toInt());
}

//void MainWindow::onSpeedCtrlChanged()
//{
//    QVector<QString> speedLevelCaster = { "0.6","0.75","0.9","1.0","1.1","1.25","1.4" };
//    startTempoChanging(speedLevelCaster.at(ui.ctrl_speed->currentIndex()));
//}

//void MainWindow::startTempoChanging(QString newTempo)
//{
//    QStringList args;
//    args.append("-i");
//    args.append(oriFileName);
//    args.append("-af");
//    args.append("atempo=" + newTempo);
//    args.append(QCoreApplication::applicationDirPath() + "/current_media_newtempo.mp3");
//
//    QFile file;
//    file.setFileName(QCoreApplication::applicationDirPath() + "/current_media_newtempo.mp3");
//    file.remove();
//    file.setFileName(oriFileName);
//    if (!file.exists())
//    {
//        ui.statusbar->setText("无法转换速度：源文件已更改");
//        return;
//    }
//
//    ui.ctrl_speed->setEnabled(false);
//
//    connect(ffmpegTemopChanger, &QProcess::finished, this, &MainWindow::onTempoChangeFinished);
//    ffmpegTemopChanger->start("ffmpeg.exe", args);
//
//    if (ffmpegTemopChanger->state() != QProcess::Running)
//    {
//        QMessageBox::warning(NULL, "FFmpeg调用失败", ffmpegTemopChanger->errorString());
//        ui.ctrl_speed->setEnabled(true);
//        return;
//    }
//
//    ui.statusbar->setText("正在转换速度...");
//}

//void MainWindow::onTempoChangeFinished()
//{
//    bool playStatusRem = corePlayer->isPlaying();
//    
//    //QMessageBox::information(NULL, "调试信息", ffmpegTemopChanger->readAllStandardError());
//    corePlayer->stop();
//    corePlayer->setSource(QUrl());
//    QFile fileUpdater;
//    fileUpdater.setFileName(QCoreApplication::applicationDirPath() + "/current_media");
//    fileUpdater.remove();
//    fileUpdater.setFileName(QCoreApplication::applicationDirPath() + "/current_media_newtempo.mp3");
//    fileUpdater.rename(QCoreApplication::applicationDirPath() + "/current_media");
//    
//    clearTimezoneTable();
//    ui.statusbar->setText("重新加载文件...");
//    playerReloadFile();
//
//    if (playStatusRem)
//        corePlayer->play();
//    else
//        corePlayer->pause();
//
//    ui.ctrl_speed->setEnabled(true);
//}

void MainWindow::clearTimezoneTable()
{
    while (ui.HL_timezone->count() > 0)
    {
        QLayoutItem* it = ui.HL_timezone->itemAt(0);
        it->widget()->deleteLater();
        ui.HL_timezone->removeItem(it);
    }
}

void MainWindow::paintParts(QVector<PartType> partsType)
{
    for (size_t i = 0; i < ui.HL_timezone->count(); i++)
    {
        QLayoutItem* it = ui.HL_timezone->itemAt(i);
        if(partsType.at(i)==part_description)
            it->widget()->setProperty("partType","description");
        if (partsType.at(i) == part_1)
            it->widget()->setProperty("partType", "part1");
        if (partsType.at(i) == part_2)
            it->widget()->setProperty("partType", "part2");
        //ui.HL_timezone->removeItem(it);
    }

}

void MainWindow::setWindowStaytopStatus()
{
    if (ui.wctrl_staytop->isChecked())
    {
        this->setWindowFlag(Qt::WindowStaysOnTopHint, true);
        setWindowOpacity(0.5);
    }
    else
    {
        this->setWindowFlag(Qt::WindowStaysOnTopHint, false);
        setWindowOpacity(1);
    }
        
    this->show();
}

void MainWindow::mousePressEvent(QMouseEvent* event)
{
    if (event->pos().y() <= 80)
        windowDiffPos = event->globalPos() - this->pos();
}

void MainWindow::mouseMoveEvent(QMouseEvent* event)
{
    if (event->pos().y() <= 80)
        this->move(event->globalPos() - windowDiffPos);
}