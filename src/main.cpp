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

#include <QtGui/QApplication>
#include "mainwindow.h"

#include <signal.h>
#include <unistd.h>

#include "MyDaemon.h"

static int setup_unix_signal_handlers()
{
    struct sigaction hup, term;

    hup.sa_handler = MyDaemon::hupSignalHandler;
    sigemptyset(&hup.sa_mask);
    hup.sa_flags = 0;
    hup.sa_flags |= SA_RESTART;

    if (sigaction(SIGHUP, &hup, 0) > 0)
        return 1;

    term.sa_handler = MyDaemon::termSignalHandler;
    sigemptyset(&term.sa_mask);
    term.sa_flags |= SA_RESTART;

    if (sigaction(SIGTERM, &term, 0) > 0)
        return 2;

    return 0;
}


/**
 * http://doc.trolltech.com/4.6/unix-signals.html
 */

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("www.sourceforge.net");
    app.setApplicationName("PedometerLog");
    app.setGraphicsSystem("raster");

    MainWindow MainWin;
    MainWin.show();

    setup_unix_signal_handlers();

    MyDaemon daemon(&MainWin);

    return app.exec();
}
