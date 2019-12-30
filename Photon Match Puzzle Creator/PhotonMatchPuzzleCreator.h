/*
This file is part of Photon Match Puzzle Creator.

	Photon Match Puzzle Creator is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Photon Match Puzzle Creator is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Photon Match Puzzle Creator.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_PhotonMatchPuzzleCreator.h"
#include <memory>
#include <QProcess>
#include <QShortcut>
#include <QCompleter>

class PhotonMatchPuzzleCreator : public QMainWindow
{
	Q_OBJECT

public:
	PhotonMatchPuzzleCreator(QWidget *parent = Q_NULLPTR);
	void closeEvent(QCloseEvent *event);

private:
	Ui::PhotonMatchPuzzleCreatorClass ui;

	const QString appExecutablePath = QCoreApplication::applicationDirPath();
	const QString winTitleProgramName = "Photon Match Puzzle Creator";
	const QString winTitlePlaceholder = "[*]";
	const QString winTitleUntitled = "Untitled Project";
	QString fileCurrent = "";
	QString fileDirLastOpened = appExecutablePath;
	QString fileDirLastSaved = appExecutablePath;
	QString bal4webPath;
	QString creatorName;
	std::map<QString, QString> languageCodeTable;
	std::unique_ptr<QCompleter> languageListCompleter = std::make_unique<QCompleter>(this);

	std::unique_ptr<QProcess> process = std::make_unique<QProcess>();
	std::unique_ptr<QShortcut> shortcutPasteAcrossBoxes = std::make_unique<QShortcut>(QKeySequence(tr("Ctrl+Shift+V", "Paste Across")), this);

	void prefLoad();
	void prefSave();

	enum class ExtractSubstringStyle { EXTRACT_FIRST, EXTRACT_ALL };
	std::string extractSubstringInbetween(const std::string strBegin, const std::string strEnd, const std::string &strExtractFrom);
	QString extractSubstringInbetweenQt(const QString strBegin, const QString strEnd, const QString &strExtractFrom);
	QString extractSubstringInbetweenQt(const QString strBegin, const QString strEnd, const QString &strExtractFrom, ExtractSubstringStyle extractSubstringStyle);
	QString decodeHtmlEntities(const QString strToDecode);

	// Set to TRUE for High Bit version and FALSE for normal version
	QString generateHash64HexOnly(const std::string &strToHash, bool highBit);

private slots:
	void statusBarInit();
	void fileDocumentModified();
	void fileSetCurrent(const QString &filename);
	void fileNew();
	void fileOpen();
	bool fileSaveModifCheck();
	bool fileSaveOps();
	bool fileSave(const QString &filename);
	bool fileSaveAs();
	void clearEntryForms();
};
