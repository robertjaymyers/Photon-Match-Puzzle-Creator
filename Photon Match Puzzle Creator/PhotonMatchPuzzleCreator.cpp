#include "PhotonMatchPuzzleCreator.h"
#include <QDebug>
#include <QFileInfo>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QInputDialog>
#include <QTextCursor>
#include <QClipboard>
#include <QTextBlock>

PhotonMatchPuzzleCreator::PhotonMatchPuzzleCreator(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	ui.mainToolBar->setVisible(false);

	setWindowTitle(winTitleUntitled + winTitlePlaceholder + " - " + winTitleProgramName);
	statusBarInit();

	if (QApplication::arguments().size() > 1)
	{
		const QString filename = QApplication::arguments().at(1);
		if (!filename.isEmpty())
		{
			clearEntryForms();

			QString fileContents;
			QFile fileRead(filename);
			if (fileRead.open(QIODevice::ReadOnly))
			{
				QTextStream qStream(&fileRead);
				while (!qStream.atEnd())
				{
					QString line = qStream.readLine();
					if (line.contains("langNameEntryLeft="))
						ui.langNameEntryLeft->setText(QString::fromStdString(extractSubstringInbetween("=", "", line.toStdString())));
					else if (line.contains("langNameEntryRight="))
						ui.langNameEntryRight->setText(QString::fromStdString(extractSubstringInbetween("=", "", line.toStdString())));
					else if (line.contains("catNameEntry="))
						ui.catNameEntry->setText(QString::fromStdString(extractSubstringInbetween("=", "", line.toStdString())));
					else if (line.contains("wordPairsEntryLeft="))
						ui.wordPairsEntryLeft->insertPlainText(QString::fromStdString(extractSubstringInbetween("=", "", line.toStdString())).replace(",", "\n"));
					else if (line.contains("wordPairsEntryRight="))
						ui.wordPairsEntryRight->insertPlainText(QString::fromStdString(extractSubstringInbetween("=", "", line.toStdString())).replace(",", "\n"));
				}
			}
			fileRead.close();
			fileSetCurrent(filename);
			statusBar()->showMessage(tr("File loaded"), 2000);
			fileDirLastOpened = QFileInfo(filename).path();
		}
	}

	prefLoad();
	ui.actionSetbal4webPath->setStatusTip("Current: " + bal4webPath);
	ui.actionSetCreatorName->setStatusTip("Current: " + creatorName);

	connect(ui.wordPairsEntryLeft, &QPlainTextEdit::textChanged, this, &PhotonMatchPuzzleCreator::fileDocumentModified);
	connect(ui.wordPairsEntryRight, &QPlainTextEdit::textChanged, this, &PhotonMatchPuzzleCreator::fileDocumentModified);
	connect(ui.langNameEntryLeft, &QLineEdit::textChanged, this, &PhotonMatchPuzzleCreator::fileDocumentModified);
	connect(ui.langNameEntryRight, &QLineEdit::textChanged, this, &PhotonMatchPuzzleCreator::fileDocumentModified);
	connect(ui.catNameEntry, &QLineEdit::textChanged, this, &PhotonMatchPuzzleCreator::fileDocumentModified);

	connect(ui.actionNew, &QAction::triggered, this, &PhotonMatchPuzzleCreator::fileNew);
	connect(ui.actionOpen, &QAction::triggered, this, &PhotonMatchPuzzleCreator::fileOpen);
	connect(ui.actionSave, &QAction::triggered, this, &PhotonMatchPuzzleCreator::fileSaveOps);
	connect(ui.actionSaveAs, &QAction::triggered, this, &PhotonMatchPuzzleCreator::fileSaveAs);
	connect(ui.actionExit, &QAction::triggered, this, &PhotonMatchPuzzleCreator::close);

	ui.actionNew->setShortcut(Qt::Key_N | Qt::ControlModifier);
	ui.actionOpen->setShortcut(Qt::Key_O | Qt::ControlModifier);
	ui.actionSave->setShortcut(Qt::Key_S | Qt::ControlModifier);
	ui.actionSaveAs->setShortcut(Qt::Key_S | Qt::ControlModifier | Qt::ShiftModifier);

	connect(ui.actionSetbal4webPath, &QAction::triggered, this, [=]() {
		QString dir = QFileDialog::getExistingDirectory(this, tr("Select Folder"), "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
		if (!dir.isEmpty())
		{
			bal4webPath = dir;
			ui.actionSetbal4webPath->setStatusTip("Current: " + bal4webPath);
		}
	});

	connect(ui.actionSetCreatorName, &QAction::triggered, this, [=]() {
		bool ok;
		QString newName = QInputDialog::getText(this, tr("Choose Creator Name"), tr("Creator Name:"), QLineEdit::Normal, creatorName, &ok, Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::MSWindowsFixedSizeDialogHint);
		if (ok && !newName.isEmpty())
		{
			creatorName = newName;
			ui.actionSetCreatorName->setStatusTip("Current: " + creatorName);
		}
	});

	connect(ui.actionSetLanguageCodeTable, &QAction::triggered, this, [=]() {
		QString existingCodes;
		for (auto& i : languageCodeTable)
		{
			existingCodes.append(i.first + "=" + i.second + "\r\n");
		}
		bool ok;
		QString newCodes = QInputDialog::getMultiLineText(this, tr("Edit Language Code Table"), tr("Key/Value Pair:"), existingCodes, &ok, Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::MSWindowsFixedSizeDialogHint);
		if (ok && !newCodes.isEmpty() && existingCodes != newCodes)
		{
			languageCodeTable.clear();
			QStringList newCodesList = newCodes.split('\n', QString::SkipEmptyParts);
			for (auto& i : newCodesList)
			{
				QString langCodeKey = QString::fromStdString(extractSubstringInbetween("", "=", i.toStdString()));
				QString langCodeVal = QString::fromStdString(extractSubstringInbetween("=", "", i.toStdString()));
				languageCodeTable.insert(std::pair<QString, QString>(langCodeKey, langCodeVal));
			}
		}
	});

	connect(this->process.get(), qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this, [=]() {
		QCoreApplication::processEvents();
		if (process.get()->exitCode() != 0)
		{
			qDebug() << QString::number(process.get()->exitCode());
			qDebug() << process.get()->readAllStandardError();
		}
		else
		{
			qDebug("Processed successfully.");
		}
	});

	connect(ui.actionExportWordPairs, &QAction::triggered, this, [=]() {
		if (ui.langNameEntryLeft->text().isEmpty() ||
			ui.langNameEntryRight->text().isEmpty() ||
			ui.catNameEntry->text().isEmpty() || 
			ui.wordPairsEntryLeft->document()->isEmpty() ||
			ui.wordPairsEntryRight->document()->isEmpty() ||
			creatorName.isEmpty() || 
			bal4webPath.isEmpty())
		{
			QMessageBox::warning(this, tr("Export Failed"), "One or more required fields is empty. Make sure to set bal4web path, creator name, and fill out all of the input fields.");
			return;
		}

		// Word Pairs Export
		const QStringList wordPairsEntryLeftList = ui.wordPairsEntryLeft->toPlainText().split('\n', QString::SkipEmptyParts);
		const QStringList wordPairsEntryRightList = ui.wordPairsEntryRight->toPlainText().split('\n', QString::SkipEmptyParts);
		const QStringList wordPairsEntryCombinedList = wordPairsEntryLeftList + wordPairsEntryRightList;

		if (wordPairsEntryLeftList.length() != wordPairsEntryRightList.length())
		{
			QMessageBox::warning(this, tr("Export WordPairs Canceled"), "The word pair forms must have an equal number of entries.");
			return;
		}
		const QString compositeFieldNames = creatorName + ui.langNameEntryLeft->text() + ui.langNameEntryRight->text() + ui.catNameEntry->text() + "Dictionary";
		const QString wordPairsTextFileName = creatorName + "_" + generateHash64HexOnly(compositeFieldNames.toStdString(), true);
		{
			QString compositePath = appExecutablePath + "/WordPairs/";
			compositePath.append(ui.langNameEntryLeft->text() + "-" + ui.langNameEntryRight->text());
			compositePath.append("/" + ui.catNameEntry->text());
			QDir compositeDir(compositePath);
			compositeDir.mkpath(".");

			compositePath.append("/" + wordPairsTextFileName + ".txt");

			QFile fileWrite(compositePath);
			if (fileWrite.open(QIODevice::WriteOnly))
			{
				QTextStream qStream(&fileWrite);
				qStream.setCodec("UTF-16BE");
				qStream.setGenerateByteOrderMark(true);

				for (int i = 0; i < wordPairsEntryLeftList.length(); i++)
				{
					qStream << "[id]" + generateHash64HexOnly(wordPairsEntryLeftList[i].toStdString() + ui.langNameEntryLeft->text().toStdString() + creatorName.toStdString(), true) + "[/id]" + wordPairsEntryLeftList[i];
					qStream << ",";
					qStream << "[id]" + generateHash64HexOnly(wordPairsEntryRightList[i].toStdString() + ui.langNameEntryRight->text().toStdString() + creatorName.toStdString(), true) + "[/id]" + wordPairsEntryRightList[i];
					qStream << "\r\n";
				}
			}
			fileWrite.close();
		}

		// TextToSpeech Export
		if (ui.actionIncludeTextToSpeech->isChecked())
		{
			if (languageCodeTable[ui.langNameEntryLeft->text()].isEmpty() ||
				languageCodeTable[ui.langNameEntryRight->text()].isEmpty())
			{
				QMessageBox::warning(this, tr("Export TextToSpeech Canceled"), "One or more of the given languages was not found in the language code table.\r\nLanguage names need to correspond to a locale (ex: French=fr-FR)\r\nYou can add to the language code table under export settings, or manually edit the file in a text editor.");
				return;
			}

			QString compositePath = appExecutablePath + "/TextToSpeech/";
			compositePath.append(ui.langNameEntryLeft->text() + "-" + ui.langNameEntryRight->text());
			compositePath.append("/" + ui.catNameEntry->text());
			compositePath.append("/" + wordPairsTextFileName);
			QDir compositeDir(compositePath);
			compositeDir.mkpath(".");

			ui.ttsExportLog->clear();
			int entriesRemainingNum = wordPairsEntryLeftList.length() + wordPairsEntryRightList.length();
			int entriesProcessed = 0;
			int entriesIgnored = 0;
			const QString langValLeft = languageCodeTable[ui.langNameEntryLeft->text()];
			for (auto& word : wordPairsEntryLeftList)
			{
				QString wordOutputPath = compositePath;
				wordOutputPath.append("/" + generateHash64HexOnly(word.toStdString() + ui.langNameEntryLeft->text().toStdString() + creatorName.toStdString(), true) + ".wav");
				if (!QFile::exists(wordOutputPath))
				{
					process.get()->start("C:/WINDOWS/system32/cmd.exe", QStringList() << "/C" << bal4webPath + "/bal4web.exe" << "-t" << word << "-w" << wordOutputPath << "-s" << "Google" << "-l" << langValLeft);
					process.get()->waitForFinished();
					ui.ttsExportLog->append(word + " wav file created. " + QString::number(entriesRemainingNum) + " files left to process.");
					this->repaint();
					entriesRemainingNum--;
					entriesProcessed++;
				}
				else
				{
					ui.ttsExportLog->append("An audio file with " + word + "'s hash already exists.");
					entriesIgnored++;
				}
			}
			const QString langValRight = languageCodeTable[ui.langNameEntryRight->text()];
			for (auto& word : wordPairsEntryRightList)
			{
				QString wordOutputPath = compositePath;
				wordOutputPath.append("/" + generateHash64HexOnly(word.toStdString() + ui.langNameEntryRight->text().toStdString() + creatorName.toStdString(), true) + ".wav");
				if (!QFile::exists(wordOutputPath))
				{
					process.get()->start("C:/WINDOWS/system32/cmd.exe", QStringList() << "/C" << bal4webPath + "/bal4web.exe" << "-t" << word << "-w" << wordOutputPath << "-s" << "Google" << "-l" << langValRight);
					process.get()->waitForFinished();
					ui.ttsExportLog->append(word + " wav file created. " + QString::number(entriesRemainingNum) + " files left to process.");
					entriesRemainingNum--;
					entriesProcessed++;
				}
				else
				{
					ui.ttsExportLog->append("An audio file with " + word + "'s hash already exists.");
					entriesRemainingNum--;
					entriesIgnored++;
				}
			}
			ui.ttsExportLog->append("TTS processing finished. You can now safely close the program.");
			ui.ttsExportLog->append(QString::number(entriesProcessed) + " entries processed.");
			ui.ttsExportLog->append(QString::number(entriesIgnored) + " entries ignored due to a file of the given name already existing.");
		}
	});

	connect(shortcutPasteAcrossBoxes.get(), &QShortcut::activated, this, [=]() {
		QTextCursor cursor(ui.wordPairsEntryLeft->textCursor());
		cursor.beginEditBlock();
		QString clipboardText = QApplication::clipboard()->text();
		QStringList clipboardTextStrList = clipboardText.split('\n', QString::SkipEmptyParts);
		for (auto& i : clipboardTextStrList)
		{
			QString leftStr = extractSubstringInbetweenQt("", ",", i);
			QString rightStr = extractSubstringInbetweenQt(",", "", i);

			qDebug() << cursor.blockNumber();

			if (ui.wordPairsEntryLeft->document()->findBlockByLineNumber(cursor.blockNumber()).text().isEmpty())
				ui.wordPairsEntryLeft->insertPlainText(leftStr + "\n");
			else
				ui.wordPairsEntryLeft->insertPlainText("\n" + leftStr + "\n");

			if (ui.wordPairsEntryRight->document()->findBlockByLineNumber(cursor.blockNumber()).text().isEmpty())
				ui.wordPairsEntryRight->insertPlainText(rightStr + "\n");
			else
				ui.wordPairsEntryRight->insertPlainText("\n" + rightStr + "\n");
		}
		cursor.endEditBlock();
	});
}

void PhotonMatchPuzzleCreator::closeEvent(QCloseEvent *event)
{
	if (fileSaveModifCheck())
	{
		prefSave();
		event->accept();
	}
	else
	{
		event->ignore();
	}
}

void PhotonMatchPuzzleCreator::statusBarInit()
{
	statusBar()->showMessage(tr("Ready"));
}

void PhotonMatchPuzzleCreator::fileDocumentModified()
{
	setWindowModified(true);
}

void PhotonMatchPuzzleCreator::fileSetCurrent(const QString &filename)
{
	fileCurrent = filename;
	setWindowModified(false);

	QString displayName;
	if (fileCurrent.isEmpty())
	{
		displayName = winTitleUntitled;
	}
	else
	{
		displayName = QFileInfo(fileCurrent).fileName();
	}
	setWindowTitle(displayName + winTitlePlaceholder + " - " + winTitleProgramName);
}

void PhotonMatchPuzzleCreator::fileNew()
{
	if (fileSaveModifCheck())
	{
		clearEntryForms();
		fileSetCurrent(QString());
	}
}

void PhotonMatchPuzzleCreator::fileOpen()
{
	if (fileSaveModifCheck())
	{
		QString filename = QFileDialog::getOpenFileName(this, tr("Open"), fileDirLastOpened, tr("Photon Match Project Files (*.photonmatchproj)"));
		if (!filename.isEmpty())
		{
			clearEntryForms();

			QString fileContents;
			QFile fileRead(filename);
			if (fileRead.open(QIODevice::ReadOnly))
			{
				QTextStream qStream(&fileRead);
				while (!qStream.atEnd())
				{
					QString line = qStream.readLine();
					if (line.contains("langNameEntryLeft="))
						ui.langNameEntryLeft->setText(QString::fromStdString(extractSubstringInbetween("=", "", line.toStdString())));
					else if (line.contains("langNameEntryRight="))
						ui.langNameEntryRight->setText(QString::fromStdString(extractSubstringInbetween("=", "", line.toStdString())));
					else if (line.contains("catNameEntry="))
						ui.catNameEntry->setText(QString::fromStdString(extractSubstringInbetween("=", "", line.toStdString())));
					else if (line.contains("wordPairsEntryLeft="))
						ui.wordPairsEntryLeft->insertPlainText(QString::fromStdString(extractSubstringInbetween("=", "", line.toStdString())).replace(",", "\n"));
					else if (line.contains("wordPairsEntryRight="))
						ui.wordPairsEntryRight->insertPlainText(QString::fromStdString(extractSubstringInbetween("=", "", line.toStdString())).replace(",", "\n"));
				}
			}
			fileRead.close();
			fileSetCurrent(filename);
			statusBar()->showMessage(tr("File loaded"), 2000);
			fileDirLastOpened = QFileInfo(filename).path();
		}
	}
}

bool PhotonMatchPuzzleCreator::fileSaveModifCheck()
{
	if (!isWindowModified())
		return true;

	const QMessageBox::StandardButton ret
		= QMessageBox::warning(this, tr("Photon Match Puzzle Creator"),
			tr("The project has been modified.\nDo you want to save your changes?"),
			QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

	switch (ret)
	{
	case QMessageBox::Save:
		return fileSaveOps();
	case QMessageBox::Cancel:
		return false;
	default:
		break;
	}

	return true;
}

bool PhotonMatchPuzzleCreator::fileSaveOps()
{
	if (fileCurrent.isEmpty())
	{
		return fileSaveAs();
	}
	else
	{
		return fileSave(fileCurrent);
	}
}

bool PhotonMatchPuzzleCreator::fileSave(const QString &filename)
{
	// Save into plain text file (with UTF-16 BE encoding) and give it extension .photonmatchproj
	// Save each form on a separate line, i.e.
	// 1 langNameEntryLeft=English
	// 2 langNameEntryRight=French
	// 3 catNameEntry=Animals
	// 4 wordPairsEntryLeft=crab,girl
	// 5 wordPairsEntryRight=le crabe,la fille

	// This way it's simple to load back in. Don't forget to use \r\n for backwards compatibility in windows.

	const QString linebreakStr = "\r\n";
	QFile fileWrite(filename);
	if (fileWrite.open(QIODevice::WriteOnly))
	{
		QTextStream qStream(&fileWrite);
		qStream << "langNameEntryLeft=" + ui.langNameEntryLeft->text();
		qStream << linebreakStr;
		qStream << "langNameEntryRight=" + ui.langNameEntryRight->text();
		qStream << linebreakStr;
		qStream << "catNameEntry=" + ui.catNameEntry->text();
		qStream << linebreakStr;
		qStream << "wordPairsEntryLeft=";
		QStringList wordPairsEntryLeftList = ui.wordPairsEntryLeft->toPlainText().split('\n', QString::SkipEmptyParts);
		for (auto& i : wordPairsEntryLeftList)
			qStream << i + ",";
		qStream << linebreakStr;
		qStream << "wordPairsEntryRight=";
		QStringList wordPairsEntryRight = ui.wordPairsEntryRight->toPlainText().split('\n', QString::SkipEmptyParts);
		for (auto& i : wordPairsEntryRight)
			qStream << i + ",";
		fileWrite.close();
	}

	fileSetCurrent(filename);
	statusBar()->showMessage(tr("File saved"), 2000);
	fileDirLastSaved = QFileInfo(filename).path();
	return true;
}

bool PhotonMatchPuzzleCreator::fileSaveAs()
{
	QFileDialog dialog(this, tr("Save As"), fileDirLastSaved, tr("Photon Match Project Files (*.photonmatchproj)"));
	dialog.setWindowModality(Qt::WindowModal);
	dialog.setAcceptMode(QFileDialog::AcceptSave);
	if (dialog.exec() != QFileDialog::Accepted)
		return false;
	return fileSave(dialog.selectedFiles().first());
}

void PhotonMatchPuzzleCreator::clearEntryForms()
{
	ui.wordPairsEntryLeft->clear();
	ui.wordPairsEntryRight->clear();
	ui.langNameEntryLeft->clear();
	ui.langNameEntryRight->clear();
	ui.catNameEntry->clear();
}

void PhotonMatchPuzzleCreator::prefLoad()
{
	{
		QFile fileRead(appExecutablePath + "/preferences.txt");
		if (fileRead.open(QIODevice::ReadOnly))
		{
			QTextStream contents(&fileRead);
			while (!contents.atEnd())
			{
				QString line = contents.readLine();
				if (line.contains("creatorName"))
				{
					creatorName = QString::fromStdString(extractSubstringInbetween("=", "", line.toStdString()));
				}
				else if (line.contains("bal4webPath"))
				{
					bal4webPath = QString::fromStdString(extractSubstringInbetween("=", "", line.toStdString()));
				}
				else if (line.contains("includeTextToSpeechWhenExporting"))
				{
					QString includeToggle = QString::fromStdString(extractSubstringInbetween("=", "", line.toStdString()));
					ui.actionIncludeTextToSpeech->setChecked(QVariant(includeToggle).toBool());
				}
			}
		}
		fileRead.close();
	}
	{
		QFile fileRead(appExecutablePath + "/languageCodeTable.txt");
		if (fileRead.open(QIODevice::ReadOnly))
		{
			QTextStream contents(&fileRead);
			while (!contents.atEnd())
			{
				QString line = contents.readLine();
				QString langCodeKey = QString::fromStdString(extractSubstringInbetween("", "=", line.toStdString()));
				QString langCodeVal = QString::fromStdString(extractSubstringInbetween("=", "", line.toStdString()));
				languageCodeTable.insert(std::pair<QString, QString>(langCodeKey, langCodeVal));
			}
		}
		fileRead.close();
	}
}

void PhotonMatchPuzzleCreator::prefSave()
{
	{
		QFile fileWrite(appExecutablePath + "/preferences.txt");
		if (fileWrite.open(QIODevice::WriteOnly))
		{
			QTextStream qStream(&fileWrite);
			qStream << "creatorName=" + creatorName + "\r\n";
			qStream << "bal4webPath=" + bal4webPath + "\r\n";
			qStream << "includeTextToSpeechWhenExporting=" + QString::number(ui.actionIncludeTextToSpeech->isChecked());
			fileWrite.close();
		}
	}
	{
		QFile fileWrite(appExecutablePath + "/languageCodeTable.txt");
		if (fileWrite.open(QIODevice::WriteOnly))
		{
			QTextStream qStream(&fileWrite);
			for (auto& lang : languageCodeTable)
			{
				qStream << lang.first + "=" + lang.second + "\r\n";
			}
			fileWrite.close();
		}
	}
}

std::string PhotonMatchPuzzleCreator::extractSubstringInbetween(const std::string strBegin, const std::string strEnd, const std::string &strExtractFrom)
{
	std::string extracted = "";
	int posFound = 0;

	if (!strBegin.empty() && !strEnd.empty())
	{
		while (strExtractFrom.find(strBegin, posFound) != std::string::npos)
		{
			int posBegin = strExtractFrom.find(strBegin, posFound) + strBegin.length();
			int posEnd = strExtractFrom.find(strEnd, posBegin) + 1 - strEnd.length();
			extracted.append(strExtractFrom, posBegin, posEnd - posBegin);
			posFound = posEnd;
		}
	}
	else if (strBegin.empty() && !strEnd.empty())
	{
		int posBegin = 0;
		int posEnd = strExtractFrom.find(strEnd, posBegin) + 1 - strEnd.length();
		extracted.append(strExtractFrom, posBegin, posEnd - posBegin);
		posFound = posEnd;
	}
	else if (!strBegin.empty() && strEnd.empty())
	{
		int posBegin = strExtractFrom.find(strBegin, posFound) + strBegin.length();
		int posEnd = strExtractFrom.length();
		extracted.append(strExtractFrom, posBegin, posEnd - posBegin);
		posFound = posEnd;
	}
	return extracted;
}

QString PhotonMatchPuzzleCreator::extractSubstringInbetweenQt(const QString strBegin, const QString strEnd, const QString &strExtractFrom)
{
	QString extracted = "";
	int posFound = 0;

	if (!strBegin.isEmpty() && !strEnd.isEmpty())
	{
		while (strExtractFrom.indexOf(strBegin, posFound, Qt::CaseSensitive) != -1)
		{
			int posBegin = strExtractFrom.indexOf(strBegin, posFound, Qt::CaseSensitive) + strBegin.length();
			int posEnd = strExtractFrom.indexOf(strEnd, posBegin, Qt::CaseSensitive) + 1 - strEnd.length();
			extracted += strExtractFrom.mid(posBegin, posEnd - posBegin);
			posFound = posEnd;
		}
	}
	else if (strBegin.isEmpty() && !strEnd.isEmpty())
	{
		int posBegin = 0;
		int posEnd = strExtractFrom.indexOf(strEnd, posBegin, Qt::CaseSensitive) + 1 - strEnd.length();
		extracted += strExtractFrom.mid(posBegin, posEnd - posBegin);
		posFound = posEnd;
	}
	else if (!strBegin.isEmpty() && strEnd.isEmpty())
	{
		int posBegin = strExtractFrom.indexOf(strBegin, posFound, Qt::CaseSensitive) + strBegin.length();
		int posEnd = strExtractFrom.length();
		extracted += strExtractFrom.mid(posBegin, posEnd - posBegin);
		posFound = posEnd;
	}
	return extracted;
}

// Set to TRUE for High Bit version and FALSE for normal version
QString PhotonMatchPuzzleCreator::generateHash64HexOnly(const std::string &strToHash, bool highBit)
{
	const uint64_t fnvPrime64 = 1099511628211;
	const uint64_t fnvOffsetBasis64 = 14695981039346656037;

	//QByteArray byteArray = strToHash.toUtf8();

	// Generate dec hash
	uint64_t decHash = fnvOffsetBasis64;
	int i = 0;
	int lenOfStrToHash = strToHash.length();

	for (int i = 0; i < lenOfStrToHash; i++)
	{
		decHash *= fnvPrime64;
		decHash ^= (tolower(strToHash[i]));
		//decHash ^= byteArray[i];
	}

	// Modify hash into High Bit version
	if (highBit)
	{
		unsigned long long int highBit = 0x8000000000000000;
		decHash |= highBit << 0;
	}

	QString decHashStr = QString::number(decHash);
	qDebug() << decHashStr;
	QString hexHashStr;

	// Generate hex hash
	int lenOfHex = 15;

	for (int i = lenOfHex; i >= 0; i--)
	{
		unsigned int iHexDigit = ((decHash >> (4 * i)) % 16);
		if (iHexDigit < 10)
		{
			hexHashStr.append(QString::number(iHexDigit));
		}
		else
		{
			hexHashStr.append((char)('A' + (iHexDigit - 10)));
		}
	}
	return hexHashStr;
}