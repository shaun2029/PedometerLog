/*
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MYDAEMON_H
#define MYDAEMON_H

#include <QObject>
#include <QSocketNotifier>
#include <QMainWindow>
#include "mainwindow.h"
class MyDaemon : public QObject
{
    Q_OBJECT

public:

    MyDaemon(MainWindow *parent = 0);
    ~MyDaemon();

    // Unix signal handlers.
    static void hupSignalHandler(int unused);
    static void termSignalHandler(int unused);

public slots:
    // Qt signal handlers.
    void handleSigHup();
    void handleSigTerm();

private:
    static int sighupFd[2];
    static int sigtermFd[2];

    MainWindow *mw;

    QSocketNotifier *snHup;
    QSocketNotifier *snTerm;
};

#endif // MYDAEMON_H
