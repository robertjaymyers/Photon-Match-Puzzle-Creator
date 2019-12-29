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
