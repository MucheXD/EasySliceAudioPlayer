#include "ESParser.h"

ESParser::ESParser()
{
	isSilenceDetectFinished = false;
	ffmpegProcess.setWorkingDirectory(QCoreApplication::applicationDirPath());
	raisedStd = false;
}

void ESParser::startParsing(QString fileName)
{
	isSilenceDetectFinished = false;

	QFile resultFile;
	resultFile.setFileName(QCoreApplication::applicationDirPath() + "/FFmpegResult_SpectralAnalyse.txt");
	resultFile.remove();
	resultFile.setFileName(QCoreApplication::applicationDirPath() + "/FFmpegResult_SilenceDetect.txt");
	resultFile.remove();

	currentFile = fileName;
	runSpectralAnalyse();
}

void ESParser::runSpectralAnalyse()
{
	QStringList args;
	args.append("-i");
	args.append(currentFile);
	args.append("-af");
	args.append("aspectralstats=measure=crest:win_size=49152:overlap=0.6:win_func=rect,ametadata=print:file=FFmpegResult_SpectralAnalyse.txt");
	args.append("-f");
	args.append("null");
	args.append("-");

	connect(&ffmpegProcess, &QProcess::finished, this, &ESParser::onFFmpegFinished);
	ffmpegProcess.start("ffmpeg.exe", args);
	if (ffmpegProcess.state() != QProcess::Running)
	{
		QMessageBox::warning(NULL, "FFmpeg调用失败", ffmpegProcess.errorString()
			+ "\nRunning on: " + ffmpegProcess.program());
		return;
	}
	emit parserStatusChanged(spectral_analysing);
}

void ESParser::runSilenceDetect()
{
	QStringList args;
	args.append("-i");
	args.append(currentFile);
	args.append("-af");
	args.append("silencedetect=noise=-50dB:d=4,ametadata=print:lavfi.silence_end:file=FFmpegResult_SilenceDetect.txt");
	args.append("-f");
	args.append("null");
	args.append("-");

	connect(&ffmpegProcess, &QProcess::finished, this, &ESParser::onFFmpegFinished);
	ffmpegProcess.start("ffmpeg.exe", args);
	if (ffmpegProcess.state() != QProcess::Running)
	{
		QMessageBox::warning(NULL, "FFmpeg调用失败", ffmpegProcess.errorString()
			+ "\nRunning on: " + ffmpegProcess.program());
		return;
	}
	emit parserStatusChanged(silence_detect);
}

QVector<PartType> ESParser::markParts(QVector<int32_t> parts)
{
	QString partsExpText;
	for (auto current : parts)
	{
		if (current <= 50000)
			partsExpText.append("S");
		else
			partsExpText.append("L");
	}
	
	if (markWithPattern1(partsExpText).size() == parts.size())
		return markWithPattern1(partsExpText);
	//markWithPatternN...


	return QVector<PartType>();
}

void ESParser::onFFmpegFinished()
{
	if (!isSilenceDetectFinished)
	{
		runSilenceDetect();
		isSilenceDetectFinished = true;
		return;
	}
	emit parserStatusChanged(feature_analysing);
	featureAnalyse();
}

void ESParser::featureAnalyse()
{

	//QMessageBox::information(NULL, "调试信息", ffmpegProcess.readAllStandardError());

	QFile ffmpegResult;

	ffmpegResult.setFileName(QCoreApplication::applicationDirPath() + "/FFmpegResult_SpectralAnalyse.txt");
	if (!ffmpegResult.open(QIODevice::ReadOnly))
	{
		emit parserStatusChanged(parser_onerror);
		return;
	}
	QVector<spectralAnalyseResultUnit> spectralAnalyseResult;
	while (!ffmpegResult.atEnd())
	{
		spectralAnalyseResultUnit unit;
		QString currentLine = ffmpegResult.readLine();
		if (currentLine.startsWith("lavfi.aspectralstats."))
			continue;
		if (currentLine.startsWith("frame:"))
		{
			QString timeStampString;
			bText_between(timeStampString, currentLine, "pts_time:", "\n", 0);
			unit.position = int64_t(timeStampString.toDouble() * 1000);

			currentLine = ffmpegResult.readLine();
			unit.value = int(currentLine.split("=")[1].toDouble());
			spectralAnalyseResult.append(unit);
		}
		else
		{
			emit parserStatusChanged(parser_onerror);
			return;
		}
	}

	ffmpegResult.close();
	ffmpegResult.setFileName(QCoreApplication::applicationDirPath() + "/FFmpegResult_SilenceDetect.txt");
	if(!ffmpegResult.open(QIODevice::ReadOnly))
	{
		emit parserStatusChanged(parser_onerror);
		return;
	}
	QVector<int64_t> silenceDetectResult;
	while (!ffmpegResult.atEnd())
	{
		QString currentLine = ffmpegResult.readLine();
		if (currentLine.startsWith("frame:"))
			continue;
		if (currentLine.startsWith("lavfi.silence_end"))
		{
			QString timeStampString;
			bText_between(timeStampString, currentLine, "lavfi.silence_end=", "\n", 0);
			silenceDetectResult.append(int(timeStampString.toDouble() * 1000));
		}
		else
		{
			emit parserStatusChanged(parser_onerror);
			return;
		}
	}
	ffmpegResult.close();

	raisedStd = false;
	emit ESResult(analyseSlicingPositions(spectralAnalyseResult, silenceDetectResult));
	emit parserStatusChanged(parser_finished);
}

QVector<PartType> ESParser::markWithPattern1(QString partsExpText)
{
	if (partsExpText.indexOf("SLSLSLSLSL") == -1)
		return QVector<PartType>();
	int32_t startOfPart2 = partsExpText.lastIndexOf("SLSLSLSLSL");
	if (partsExpText.mid(startOfPart2 - 6, 5) != "SSSSS")
		return QVector<PartType>();
	QVector<PartType> result;
	for (int32_t i = 0; i < partsExpText.size(); i++)
	{
		if (i < startOfPart2 - 6)
			result.append(part_description);
		else if (i < startOfPart2 - 1)
			result.append(part_1);
		else if (i == startOfPart2 - 1)
			result.append(part_description);
		else if (partsExpText.at(i) == 'S')
			result.append(part_description);
		else if (i <= startOfPart2 + 9)
			result.append(part_2);
		else result.append(part_description);
	}
	return result;
}

QVector<int64_t> ESParser::analyseSlicingPositions(QVector<spectralAnalyseResultUnit> spectralAnalyseResult,
	QVector<int64_t> silenceDetectResult)
{
	QVector<int64_t> result;
	int16_t mergeFlag = 0;
	int16_t passingStd = 800;
	if (raisedStd)
		passingStd = 1100;

	for (auto current : spectralAnalyseResult)
	{
		if (current.value >= passingStd && mergeFlag <= 0)
		{
			mergeFlag = 10;
			result.append(current.position);
		}
		else
		{
			mergeFlag -= 1;
		}
	}
	if (result.count() >= 20 && raisedStd == false)
	{
		raisedStd = true;
		return analyseSlicingPositions(spectralAnalyseResult, silenceDetectResult);
	}

	QVector<int64_t> extraPoints{};
	for (int32_t currentPos = 0;currentPos<silenceDetectResult.count();currentPos++)
	{
		bool isExistFlag = false;
		int32_t resultCount = 0;
		resultCount = result.count();
		for (int32_t comp = 0; comp < resultCount; comp++)
		{
			if (abs(silenceDetectResult.at(currentPos) - result.at(comp)) <= 2000)
			{
				isExistFlag = true;
				break;
			}
		}
		if (!isExistFlag)
			result.append(silenceDetectResult.at(currentPos));
	}
	std::sort(result.begin(), result.end());
	return result;
}

bool bText_between(QString& ret, const QString& text, QString left, QString right, int from)
{
	const int start = text.indexOf(left, from);
	const int from_left = start + left.length();
	const int end = text.indexOf(right, from_left);
	if (start == -1 || end == -1)
		return false;
	const int n = end - from_left;
	ret = text.mid(from_left, n);
	return true;
}
