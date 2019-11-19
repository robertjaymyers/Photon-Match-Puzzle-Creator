#include "PhotonMatchPuzzleCreator.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	a.setWindowIcon(QIcon(":/PhotonMatchPuzzleCreator/Icon/photon-match-puzzle-creator-program-icon.ico"));
	PhotonMatchPuzzleCreator w;
	w.show();
	return a.exec();
}
