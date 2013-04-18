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


#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <libusb.h>
#include <QSqlDatabase>
#include "qcustomplot.h"

namespace Ui {
    class MainWindow;
}

enum {X_DAILY, X_DAY_OF_WEEK, X_WEEKLY, X_MONTHLY, X_YEARLY};
enum {Y_STEPS, Y_DISTANCE, Y_KCAL, Y_WALKING_TIME, Y_METS};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void processPedometer(bool);

private slots:
    void fileimport();
    void fileexport();
    void about();
    void download();
    void plotGraph();
    void mousePlot(QCPAbstractPlottable *, QMouseEvent *);
    void mouseDoubleClickEvent(QCPAbstractPlottable *, QMouseEvent *);
    void recalibrate();
    void config();

    void addNote(void);
private:
    Ui::MainWindow *ui;
    int userReallyWantsToQuit();
    libusb_device_handle *dev_handle; // a USB device handle

    bool findDevice();
    bool openDb();
    bool createConnection();

    bool makeLog();
    bool makeNotes();
    QString getDBNote(QString);
    void updDBNote(QString);

    bool insertUpdate(QString, int, int, float, float, float, float, float, int &, int &);
    QString getDbStatus();
    void closeEvent(QCloseEvent *);
    void createActions();
    void createMenus();
    void createGraph();

    void readSettings(int &, int &);
    void writeSettings();

    void updStatus(int, int);

    void editNote(int);
    void delNote(void);

    bool maybeSave();

    int xLabelInit(QCustomPlot *, int);
    bool xLabelNeeded(QCustomPlot *, int, int);
    int xLabelCount;

    bool metric;
    bool hideMissingDays;
    bool dataMine;
    int stepSize;
    double weight;

    QDate minDate;
    QDate maxDate;

    QDate startDate;    // First date we want displayed on graph
    QDate endDate;      // Last date we want displayed on graph
    bool forceFirst;

    QString csvPath;
    QString noteDate;
    QDateTime noteBinaryDate;

    QMenu *fileMenu;
    QMenu *helpMenu;

    QAction *importAct;
    QAction *exportAct;
    QAction *downloadAct;
    QAction *exitAct;

    QAction *recalibrateAct;
    QAction *configAct;
    QAction *aboutAct;
    QAction *aboutQtAct;

    QSqlDatabase db;
    QVector<double> x,y,y2;
    QVector<int> z;

    int mainR, mainG, mainB, mainOpacity;
    int peakR, peakG, peakB, peakOpacity;

    QPoint configPos;
    QSize configSize;

    QPoint recalcPos;
    QSize recalcSize;

    QPoint notePos;
    QSize noteSize;

};

#endif // MAINWINDOW_H
