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

//http://doc.trolltech.com/4.6/unix-signals.html

#include "MyDaemon.h"
#include <QDebug>
#include <QObject>
#include <QSocketNotifier>
#include <csignal>
#include <unistd.h>
#include <sys/socket.h>
#include "mainwindow.h"

//needed to not get an undefined reference to static members
int MyDaemon::sighupFd[2];
int MyDaemon::sigtermFd[2];

MyDaemon::MyDaemon(MainWindow *parent)
    : QObject(parent)
{
    mw=parent;

    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sighupFd))
        qFatal("Couldn't create HUP socketpair");

    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigtermFd))
        qFatal("Couldn't create TERM socketpair");

    snHup = new QSocketNotifier(sighupFd[1], QSocketNotifier::Read, this);
    connect(snHup, SIGNAL(activated(int)), this, SLOT(handleSigHup()));
    snTerm = new QSocketNotifier(sigtermFd[1], QSocketNotifier::Read, this);
    connect(snTerm, SIGNAL(activated(int)), this, SLOT(handleSigTerm()));
}

MyDaemon::~MyDaemon() {}

void MyDaemon::hupSignalHandler(int)
{
    char a = '1';
    ::write(sighupFd[0], &a, sizeof(a));
}

void MyDaemon::termSignalHandler(int)
{
    char a = '2';
    ::write(sigtermFd[0], &a, sizeof(a));
}

void MyDaemon::handleSigTerm()
{
    snTerm->setEnabled(false);
    char tmp;
    ::read(sigtermFd[1], &tmp, sizeof(tmp));

    mw->processPedometer(false);

    snTerm->setEnabled(true);
}

void MyDaemon::handleSigHup()
{
    snHup->setEnabled(false);
    char tmp;
    ::read(sighupFd[1], &tmp, sizeof(tmp));

    mw->processPedometer(true);

    snHup->setEnabled(true);
}
