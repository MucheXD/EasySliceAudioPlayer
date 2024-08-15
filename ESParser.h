#pragma once
#include <QCoreApplication>
#include <QProcess>
#include <QFile>
#include <QMessagebox>
#include <QVector>
#include <QString>

enum ParserStatus
{
	spectral_analysing,
	silence_detect,
	feature_analysing,
	parser_finished,
	parser_onerror
};
enum PartType
{
	part_description,
	part_1,
	part_2
};

class ESParser : public QObject
{
	Q_OBJECT

public:

	ESParser();
	void startParsing(QString fileName);
	void runSpectralAnalyse();
	void runSilenceDetect();
	QVector<PartType> markParts(QVector<int32_t> parts);

private:

	struct spectralAnalyseResultUnit
	{
		int64_t position;
		int32_t value;
	};

	QProcess ffmpegProcess;
	QString currentFile;
	bool isSilenceDetectFinished;
	bool raisedStd;

	void onFFmpegFinished();
	void featureAnalyse();

	QVector<PartType> markWithPattern1(QString partsExpText);

	QVector<int64_t> analyseSlicingPositions(QVector<spectralAnalyseResultUnit> spectralAnalyseResult,
		QVector<int64_t> silenceDetectResult);


signals:
	void parserStatusChanged(ParserStatus status);
	void ESResult(QVector<int64_t> slicingPositions);

};

bool bText_between(QString& ret, const QString& text, QString left, QString right, int from);