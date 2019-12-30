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
