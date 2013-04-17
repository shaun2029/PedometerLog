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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtGui>
#include <ctime>
#include <QSqlQuery>
#include <QSqlError>
#include <QDate>
#include <QMessageBox>
#include <QFileInfo>
#include "qcustomplot.h"
#include "recalculatedialog.h"
#include "configdialog.h"
#include "notedialog.h"
#include <QColorDialog>
#include <QColor>
#include <QSize>

double round(double d)
{
    return floor(d + 0.5);
}

double roundTime(double t, int mins) {
    int x=t/mins;

    return(x*mins + mins);
}


QString dayHoursMins(uint duration) {
    QString res;

    int days=duration / 1440;
    duration -= (days*1440);
    int hours=duration / 60;
    int mins = duration - hours * 60;

    if (days <=0)
        res.sprintf("%02d:%02d", hours, mins);
    else if (days == 1)
        res.sprintf("1 day %02d:%02d", hours, mins);
    else
        res.sprintf("%d days %02d:%02d", days, hours, mins);

    return(res);
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {

    ui->setupUi(this);
    forceFirst=true;

    createActions();
    createMenus();

    if (!createConnection())
        exit(1);

    if (!makeNotes())
        exit(1);

    int x,y;
    readSettings(x, y);

    createGraph();

    if (metric) {
        ui->comboBox_y->setItemText(Y_DISTANCE, "km");
    } else {
        ui->comboBox_y->setItemText(Y_DISTANCE, "Miles");
    }

    // Reinstate the users choice of X & Y scales.
    ui->comboBox_x->setCurrentIndex(x);
    ui->comboBox_y->setCurrentIndex(y);

    /*
      There is some sort of bug whereby the graph does not get the right
      geometry properties set up until the graph is replotted, so force a
      couple of replots. Then we have the size, and can calculate which
      X axis labels to drop.
      */
    int i =ui->comboBox_x->currentIndex();
    ui->comboBox_x->setCurrentIndex(0);
    show();
    ui->comboBox_x->setCurrentIndex(1);
    show();
    ui->comboBox_x->setCurrentIndex(i);

    setUnifiedTitleAndToolBarOnMac(true);

    downloadAct->setEnabled(findDevice());
    statusBar()->showMessage(getDbStatus(), 0);
}


MainWindow::~MainWindow() {
    delete ui;
}


void MainWindow::processPedometer(bool add) {

    if (add) {
        downloadAct->setEnabled(true);

        QMessageBox msgBox;

        msgBox.setText(tr("Detected a pedometer online."));
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setInformativeText("Do you want to download the data from the pedometer ?");

        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);

        int ret = msgBox.exec();

        switch (ret) {
        case QMessageBox::Yes:
            MainWindow::download();
            break;
        case QMessageBox::No:
            break;
        }
    } else {
        downloadAct->setEnabled(false);
        statusBar()->showMessage(tr("Pedometer removed from system."), 5000);
    }
}


int MainWindow::userReallyWantsToQuit() {

    QMessageBox msgBox( this->windowTitle()+tr(": Confirmation"),
                        tr("Do you really want to quit the application?"),
                        QMessageBox::Question,
                        QMessageBox::Ok,
                        QMessageBox::Cancel,
                        QMessageBox::Escape,
                        this);

    msgBox.setDefaultButton(QMessageBox::Cancel);
    msgBox.setIcon(QMessageBox::Question);

    return msgBox.exec();
}


void MainWindow::closeEvent(QCloseEvent *event) {
    if (userReallyWantsToQuit() != 2) {
        writeSettings();
        event->accept();
    } else {
        event->ignore();
    }
}


void MainWindow::createActions()
{
    importAct = new QAction(tr("&Import..."), this);
    importAct->setStatusTip(tr("Import CSV file"));
    connect(importAct, SIGNAL(triggered()), this, SLOT(fileimport()));

    exportAct = new QAction(tr("&Export..."), this);
    exportAct->setStatusTip(tr("Export database to CSV"));
    connect(exportAct, SIGNAL(triggered()), this, SLOT(fileexport()));

    downloadAct = new QAction(tr("&Download"), this);
    downloadAct->setStatusTip(tr("Download data from pedometer"));
    connect(downloadAct, SIGNAL(triggered()), this, SLOT(download()));

    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

    recalibrateAct = new QAction(tr("&Recalculate"), this);
    recalibrateAct->setStatusTip(tr("Recalculate distance & kcal based on new step size and/or weight"));
    connect(recalibrateAct, SIGNAL(triggered()), this, SLOT(recalibrate()));

    configAct = new QAction(tr("&Configuration"), this);
    configAct->setStatusTip(tr(""));
    connect(configAct, SIGNAL(triggered()), this, SLOT(config()));

    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

    aboutQtAct = new QAction(tr("About &Qt"), this);
    aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
    connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

    connect(ui->comboBox_x, SIGNAL(currentIndexChanged(int)), this, SLOT(plotGraph()));
    connect(ui->comboBox_y, SIGNAL(currentIndexChanged(int)), this, SLOT(plotGraph()));

    connect(ui->customPlot, SIGNAL(plottableClick(QCPAbstractPlottable*,QMouseEvent*)), this,
            SLOT(mousePlot(QCPAbstractPlottable *,QMouseEvent *)));

    connect(ui->customPlot, SIGNAL(plottableDoubleClick(QCPAbstractPlottable*,QMouseEvent*)), this,
            SLOT(mouseDoubleClickEvent(QCPAbstractPlottable *,QMouseEvent *)));

}


void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(importAct);
    fileMenu->addAction(exportAct);
    fileMenu->addSeparator();
    fileMenu->addAction(downloadAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    helpMenu = menuBar()->addMenu(tr("&Tools"));
    helpMenu->addAction(recalibrateAct);
    helpMenu->addSeparator();
    helpMenu->addAction(configAct);

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAct);
    helpMenu->addAction(aboutQtAct);
}


void MainWindow::createGraph() {

    ui->comboBox_x->addItem(tr("Daily"));
    ui->comboBox_x->addItem(tr("Day of Week"));
    ui->comboBox_x->addItem(tr("Weekly"));
    ui->comboBox_x->addItem(tr("Monthly"));
    ui->comboBox_x->addItem(tr("Yearly"));

    ui->comboBox_y->addItem(tr("Steps"));
    if (metric)
        ui->comboBox_y->addItem(tr("km"));
    else
        ui->comboBox_y->addItem(tr("Miles"));
    ui->comboBox_y->addItem(tr("kCal"));
    ui->comboBox_y->addItem(tr("Walking Time"));
    ui->comboBox_y->addItem(tr("METs"));
}

void MainWindow::readSettings(int &x, int &y) {

    QSettings settings("www.sourceforge.net", "PedometerLog");
    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(400, 400)).toSize();

    resize(size);
    move(pos);

    x=settings.value("x_scale", 0).toInt();
    y=settings.value("y_scale", 0).toInt();
    ui->comboBox_x->setCurrentIndex(x);
    ui->comboBox_y->setCurrentIndex(y);
    csvPath=settings.value("csvPath","").toString();
    metric=settings.value("metric",true).toBool();
    stepSize=settings.value("stepsize",-1).toInt();
    weight=settings.value("weight",-1).toDouble();
    hideMissingDays=settings.value("HideMissingDays", true).toBool();
    dataMine=settings.value("DataMine", false).toBool();

    mainR=settings.value("Main_R",150).toInt();
    mainG=settings.value("Main_G",222).toInt();
    mainB=settings.value("Main_B",0).toInt();
    mainOpacity=settings.value("Main_opacity",70).toInt();
    peakR=settings.value("Peak_R",150).toInt();
    peakG=settings.value("Peak_G",222).toInt();
    peakB=settings.value("Peak_B",0).toInt();
    peakOpacity=settings.value("Peak_opacity",70).toInt();

    configPos = settings.value("configPos", QPoint(200, 200)).toPoint();
    configSize = settings.value("configSize", QSize(198, 172)).toSize();

    recalcPos = settings.value("recalcPos", QPoint(200, 200)).toPoint();
    recalcSize = settings.value("recalcSize", QSize(190, 445)).toSize();

    notePos = settings.value("notePos", QPoint(200, 200)).toPoint();
    noteSize = settings.value("noteSize", QSize(198, 172)).toSize();
}

void MainWindow::writeSettings() {

    QSettings settings("www.sourceforge.net", "PedometerLog");

    settings.setValue("pos", pos());
    settings.setValue("size", size());
    settings.setValue("x_scale",ui->comboBox_x->currentIndex());
    settings.setValue("y_scale",ui->comboBox_y->currentIndex());
    settings.setValue("csvPath", csvPath);
    settings.setValue("metric", metric);
    settings.setValue("HideMissingDays", hideMissingDays);
    settings.setValue("DataMine", dataMine);
    settings.setValue("stepsize", stepSize);
    settings.setValue("weight", weight);
    settings.setValue("Main_R", mainR);
    settings.setValue("Main_G", mainG);
    settings.setValue("Main_B", mainB);
    settings.setValue("Main_opacity", mainOpacity);
    settings.setValue("Peak_R", peakR);
    settings.setValue("Peak_G", peakG);
    settings.setValue("Peak_B", peakB);
    settings.setValue("Peak_opacity", peakOpacity);

    settings.setValue("configPos", configPos);
    settings.setValue("configSize", configSize);

    settings.setValue("recalcPos", recalcPos);
    settings.setValue("recalcSize", recalcSize);

    settings.setValue("notePos", notePos);
    settings.setValue("noteSize", noteSize);
}

void MainWindow::fileimport() {

    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                    csvPath,
                                                    tr("Files (*.CSV)"));
    if (fileName.size() == 0)
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QFileInfo fileext(fileName);
    csvPath=fileext.absolutePath();

    int added, updated;
    added=updated=0;

    while (!file.atEnd()) {
        QString line = file.readLine();

        QStringList vals = line.split(',');

        if (!vals.isEmpty()) {
            if (QString::compare("Date", vals[0], Qt::CaseInsensitive) != 0) {
                QString Date = vals[0];
                int steps = vals[1].toInt();
                int targetStep= vals[2].toInt();
                float stepDistance = vals[3].toFloat();
                float weight = vals[4].toFloat();
                float distance = vals[5].toFloat();
                float calories = vals[6].toFloat();
                float walkingTime = vals[7].toFloat();

                insertUpdate(Date, steps, targetStep, stepDistance, weight, distance, calories, walkingTime, added, updated);
            }
        }
    }

    file.close();
    updStatus(added, updated);
    plotGraph();
}


void MainWindow::fileexport() {

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"),
                                                    csvPath,
                                                    tr("*.csv"));
    if (fileName.size() == 0)
        return;

    // If there is no extension, add .csv
    QFileInfo fileext(fileName);
    if(fileext.suffix().isEmpty()) fileName += ".csv";

    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Application"),
                             tr("Cannot write file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return;
    }
    csvPath=fileext.absolutePath();

    QTextStream out(&file);

    if (metric)
        out << "Date,Steps,Target Step,Step Distance (cm),Weight (kg),Distance (km),Calories (kcal),Walking Time (Mins)" << endl;
    else
        out << "Date,Steps,Target Step,Step Distance (inch),Weight (lbs),Distance (Miles),Calories (kcal),Walking Time (Mins)" << endl;

    QSqlQuery query;
    query.prepare("select * from log order by date");

    if (!query.exec())
    {
        qDebug("error executing: %s", qPrintable(query.lastError ().text() ) );
        return;
    }

    while (query.next()) {
        QDateTime date=query.value(0).toDateTime();
        QString date_str = date.toString("yyyy-MM-dd");

        int steps=query.value(1).toInt();
        int steps_target=query.value(2).toInt();

        float stepDistance;
        float weight;
        float distance;

        if (metric) {
            stepDistance = query.value(3).toFloat();
            weight = query.value(4).toFloat();
            distance = query.value(5).toFloat();
        } else {
            stepDistance = query.value(3).toFloat() * 0.393700787;
            weight = query.value(4).toFloat() * 2.20462;
            distance = query.value(5).toFloat() * 0.621371192;
        }
        float calories = query.value(6).toFloat();
        float walkingTime = query.value(7).toFloat();

        out << date_str << "," << steps << "," << steps_target << ","
            << stepDistance << "," << weight << "," << distance << ","
            << calories << "," << walkingTime << endl;

    }
    file.close();

    statusBar()->showMessage(tr("File saved"), 2000);
}


void MainWindow::plotGraph() {

    QSqlQuery query;
    int i;
    int y_precision=0;

    QDate firstDate;
    QDateTime date;

    query.prepare("select min(date) from log where steps <> 0");
    if (!query.exec()) {
        qDebug("error executing: %s", qPrintable(query.lastError().text() ) );
        return;
    }
    query.next();
    date=query.value(0).toDateTime();
    firstDate=date.date();
    //qDebug() << "First Date" << firstDate;

    if (!dataMine || forceFirst) {
        startDate=firstDate;
        endDate=QDate::currentDate();
    } else {
        if (firstDate < startDate) {
            firstDate = startDate;
        }
    }

    switch(ui->comboBox_y->currentIndex()) {
    case Y_STEPS:
        query.prepare("select date, steps, target from log where steps <> 0 and date >= '" + startDate.toString("yyyy-MM-dd") + "' and date <= '" + endDate.toString("yyyy-MM-dd") + "' order by date");
        break;
    case Y_DISTANCE:
        y_precision=1;
        query.prepare("select date, km from log where steps <> 0 and date >= '" + startDate.toString("yyyy-MM-dd") + "' and date <= '" + endDate.toString("yyyy-MM-dd") + "'order by date");
        break;
    case Y_KCAL:
        y_precision=1;
        query.prepare("select date, kcal from log where steps <> 0 and date >= '" + startDate.toString("yyyy-MM-dd") + "' and date <= '" + endDate.toString("yyyy-MM-dd") + "'order by date");
        break;
    case Y_WALKING_TIME:
        query.prepare("select date, walkingtime from log where steps <> 0 and date >= '" + startDate.toString("yyyy-MM-dd") + "' and date <= '" + endDate.toString("yyyy-MM-dd") + "'order by date");
        break;
    case Y_METS:
        y_precision=2;
        query.prepare("select date, kcal, weight, walkingtime from log where steps <> 0 and date >= '" + startDate.toString("yyyy-MM-dd") + "' and date <= '" + endDate.toString("yyyy-MM-dd") + "'order by date");
        break;
    default:
        return;
    }

    if (!query.exec()) {
        qDebug("error executing: %s", qPrintable(query.lastError ().text() ) );
        return;
    }

    QDate date2;

    x.resize(50);
    y.resize(50);
    y2.resize(50);
    z.resize(50);
    for (i=0; i<x.size(); i++) {
        x[i]=y[i]=z[i]=y2[i]=0;
    }
    i=0;
    double miny, maxy, minx, maxx, value, average, target_step, temp;
    double db_weight, db_kcal, db_walkingtime;
    int lastX;
    minx=miny=9e99;
    maxx=maxy=-1;
    minDate.setDate(2099,1,1);
    maxDate.setDate(1900,1,1);

    double thisbucket, lastbucket;
    thisbucket=lastbucket=-1;
    average=0;

    bool queryLoop=query.next();
    bool gotrec=true;

    while (queryLoop) {

        if (ui->comboBox_y->currentIndex() == Y_STEPS)
            target_step=query.value(2).toDouble();
        else
            target_step=0;


        date=query.value(0).toDateTime();
        date2=date.date();

        if (!hideMissingDays) {
            if (date2 != firstDate) {
                //               qDebug() << "Need to generate a day for " << firstDate;
                //               qDebug() << "date2=" << date2 << " lastdate=" << firstDate;
                gotrec=true;
                date2=firstDate;
                QDateTime mydate(firstDate);
                date=mydate;
            } else
                gotrec=false;
        } else
            gotrec=false;

        if (date2 < minDate)
            minDate=date2;

        if (date2 > maxDate)
            maxDate=date2;

        if (gotrec) {
            value=0;
            db_kcal=0;
            db_weight=0;
            db_walkingtime=0;

        } else {
            // If we want miles, give miles.
            if (!metric && ui->comboBox_y->currentIndex()==Y_DISTANCE)
                value=query.value(1).toDouble() * 0.621371192;
            else
                value=query.value(1).toDouble();

            if (ui->comboBox_y->currentIndex()==Y_METS) {
                db_kcal=query.value(1).toDouble();
                db_weight=query.value(2).toDouble();
                db_walkingtime=query.value(3).toDouble();
                value=(db_kcal / db_weight) / (db_walkingtime/60.0);
            }
        }
        switch(ui->comboBox_x->currentIndex()) {
        case X_DAILY:
            thisbucket=date.toTime_t();
            break;
        case X_DAY_OF_WEEK:
            lastbucket=-1;
            thisbucket=i=date2.dayOfWeek() -1;
            break;
        case X_WEEKLY:
            date2 = date2.addDays( - (date2.dayOfWeek() - 1)); // Mondays
            date.setDate(date2);
            thisbucket=date.toTime_t();
            break;
        case X_MONTHLY:
            date2.setYMD(date2.year(), date2.month(), 1);
            date.setDate(date2);
            thisbucket=date.toTime_t();
            break;
        case X_YEARLY:
            thisbucket=date2.year();
        }

        if (lastbucket != -1 && lastbucket != thisbucket) {
            i++;
            if (i >= x.size()) {
                x.resize(50+i);
                y.resize(50+i);
                y2.resize(50+i);
                z.resize(50+i);
            }
            x[i]=y[i]=y2[i]=0;        // wipe previous contents
        }

        lastbucket=thisbucket;
        y[i]+=value;
        y2[i]+=target_step;
        x[i]=thisbucket;
        z[i]++;

        firstDate=firstDate.addDays(1);
        if (!gotrec)
            queryLoop=query.next();

    }

    if (ui->comboBox_x->currentIndex() == X_DAY_OF_WEEK)
        lastX=6;
    else
        lastX=i;

    for (i=0; i<=lastX; i++) {
        if (y[i] > maxy)
            maxy = y[i];
        if (y[i] < miny)
            miny = y[i];
        if (x[i] > maxx)
            maxx = x[i];
        if (x[i] < minx)
            minx = x[i];
        average+=y[i];

        if (y[i] > y2[i] && ui->comboBox_y->currentIndex() == Y_STEPS) {
            temp = y[i];
            y[i] = y2[i];
            y2[i] = temp - y[i];
        } else
            y2[i]=-1;
    }
    average/=(lastX+1);

    if (dataMine)
        ui->customPlot->setTitle("From " + startDate.toString("d-MMM-yyyy") + " to " + endDate.toString("d-MMM-yyyy"));
    else
        ui->customPlot->setTitle("");

    // Get rid of any plotables we have
    ui->customPlot->removeGraph(0);
    while (ui->customPlot->plottableCount() >= 1)
        ui->customPlot->removePlottable(0);

    QVector<double> xticks, yticks;
    QVector<QString> xlabels, ylabels;
    QPen pen;
    QCPBars *barchart = new QCPBars(ui->customPlot->xAxis, ui->customPlot->yAxis);
    QCPBars *peaks = new QCPBars(ui->customPlot->xAxis, ui->customPlot->yAxis);
    QFont legendFont = font();  // start out with MainWindow's font..
    QDateTime date3;
    QDateTime ydate;

    ui->customPlot->addPlottable(barchart);
    pen.setWidthF(1.2);
    pen.setColor(QColor(mainR, mainG, mainB));
    barchart->setPen(pen);
    barchart->setBrush(QColor(mainR, mainG, mainB, mainOpacity));

    xLabelCount=0;
    switch(ui->comboBox_x->currentIndex()) {
    case X_DAILY:         // Daily

        for (i=0; i<=lastX; i++) {
            xticks << i;
            date3=QDateTime::fromTime_t(x[i]);
            if (xLabelNeeded(ui->customPlot, 1, lastX))
                xlabels << date3.toString("d-MMM-yyyy");
            else
                xlabels << "";

        }
        break;
    case X_DAY_OF_WEEK:         // Day of Week

        xticks << 0 << 1 << 2 << 3 << 4 << 5 << 6;
        xlabels << "Mon" << "Tue" << "Wed" << "Thu" << "Fri" << "Sat" << "Sun";

        lastX=6;

        break;
    case X_WEEKLY:         // Weekly

        for (i=0; i<=lastX; i++) {
            xticks << i;

            if (xLabelNeeded(ui->customPlot, 2, lastX)) {
                date3=QDateTime::fromTime_t(x[i]);
                QString legend;
                date2= QDate::fromString(date3.toString("dd-MMM-yyyy"), "dd-MMM-yyyy");
                QTextStream(&legend) << "Week " << date2.weekNumber() << "\n" << date2.year();
                xlabels << legend;
            } else
                xlabels << "";
        }

        break;
    case X_MONTHLY:         // Monthly

        for (i=0; i<=lastX; i++) {
            xticks << i;
            if (xLabelNeeded(ui->customPlot, 2, lastX)) {
                date3=QDateTime::fromTime_t(x[i]);
                xlabels << date3.toString("MMMM\nyyyy");
            } else
                xlabels << "";
        }

        break;
    case X_YEARLY:         // Annual.

        for (i=0; i<=lastX; i++) {
            xticks << i;
            xlabels << QString::number((int)x[i]);
        }

        break;
    }

    ui->customPlot->legend->setVisible(true);

    legendFont.setPointSize(9); // and make a bit smaller for legend
    ui->customPlot->legend->setFont(legendFont);
    ui->customPlot->legend->setPositionStyle(QCPLegend::psTopLeft);
    ui->customPlot->legend->setBrush(QBrush(QColor(255,255,255,230)));

    ui->customPlot->xAxis->setTickLabelRotation(60);

    if (ui->comboBox_y->currentIndex() == Y_WALKING_TIME) {

        if (ui->comboBox_x->currentIndex() == X_DAILY) {
            miny=0; maxy=roundTime(maxy, 30);
        } else {
            int temp = miny / 60;
            miny=temp * 60; maxy=roundTime(maxy, 30);
        }

        ui->customPlot->plottable(0)->setName("Average " + dayHoursMins(average));

        ui->customPlot->yAxis->setAutoTickLabels(false);
        ui->customPlot->yAxis->setAutoTicks(false);
        ui->customPlot->yAxis->setSubTickCount(3);

        for (i=miny; i<=maxy; i+=(maxy-miny)/8) {
            ylabels << dayHoursMins(i);
            yticks << i;
        }
        ui->customPlot->yAxis->setTickVector(yticks);
        ui->customPlot->yAxis->setTickVectorLabels(ylabels);
    } else {
        miny -= miny * 0.05;
        maxy += maxy * 0.05;

        ui->customPlot->plottable(0)->setName("Average " + QString::number(average, 'f', 0));

        ui->customPlot->yAxis->setAutoTickLabels(true);
        ui->customPlot->yAxis->setAutoTicks(true);
        ui->customPlot->yAxis->setTickLabelType(QCPAxis::ltNumber);
        ui->customPlot->yAxis->setNumberFormat("f");
        ui->customPlot->yAxis->setNumberPrecision(y_precision);
    }
    ui->customPlot->yAxis->setRange(miny, maxy);

    ui->customPlot->setRangeDrag(Qt::Horizontal|Qt::Vertical);
    ui->customPlot->setRangeZoom(Qt::Horizontal|Qt::Vertical);

    ui->customPlot->xAxis->setSubTickCount(0);
    ui->customPlot->xAxis->setAutoTicks(false);
    ui->customPlot->xAxis->setAutoTickLabels(false);

    ui->customPlot->xAxis->setTickVector(xticks);
    ui->customPlot->xAxis->setTickVectorLabels(xlabels);
    ui->customPlot->xAxis->setGrid(false);
    ui->customPlot->xAxis->setRange(-1, lastX+1);

    barchart->setData(xticks, y);

    if (ui->comboBox_y->currentIndex() == Y_STEPS) {
        ui->customPlot->addPlottable(peaks);
        pen.setColor(QColor(peakR, peakG, peakB, peakOpacity));
        peaks->setPen(pen);
        peaks->setBrush(QColor(peakR, peakG, peakB, peakOpacity));

        peaks->setData(xticks, y2);

        peaks->moveAbove(barchart);
        peaks->setName("Over Target");
    }

    ui->customPlot->replot();
    statusBar()->showMessage("", 10000);
    forceFirst=false;
}

void MainWindow::mouseDoubleClickEvent(QCPAbstractPlottable *plottable, QMouseEvent *event ) {
    double key, value;
    int xIndex;
    QCPRange range;
    QDateTime date3;
    QDate date2;

    if (!dataMine) {
        QMessageBox msgBox;

        msgBox.setWindowTitle(this->windowTitle()+tr(": Warning"));
        msgBox.setInformativeText(tr("Data Mining not enabled"));
        msgBox.setIcon(QMessageBox::Warning);

        msgBox.exec();
    } else {
        //    qDebug("Mouse button %08X",event->button());

        if(event->button() & Qt::LeftButton) {
            plottable->pixelsToCoords(event->x(), event->y(), key, value);
            xIndex=round(key);
            //           qDebug() << "Double Click " << x[xIndex];
            switch(ui->comboBox_x->currentIndex()) {
            case X_DAILY:         // Daily
                date3 = QDateTime::fromTime_t(x[xIndex]);
                forceFirst=true;
                endDate = endDate.currentDate();
                ui->comboBox_x->setCurrentIndex(X_YEARLY);
                break;
            case X_WEEKLY:
                date3 = QDateTime::fromTime_t(x[xIndex]);
                startDate = date3.date();
                endDate = startDate.addDays(6);
         //       date2.setDate(startDate.year(), startDate.month(), startDate.daysInMonth());
//                qDebug() << "Date2 =" << date2 << "Start date year=" << startDate.year() << " Start Date month=" << startDate.month() << " Start date last day=" << startDate.daysInMonth();
       //         if (date2 < endDate)
       //             endDate=date2;
                ui->comboBox_x->setCurrentIndex(X_DAILY);
                break;
            case X_MONTHLY:
                date3 = QDateTime::fromTime_t(x[xIndex]);
                startDate = endDate = date3.date();
                startDate.setDate(startDate.year(), startDate.month(), 1);
                endDate.setDate(startDate.year(), startDate.month(), endDate.daysInMonth());
                ui->comboBox_x->setCurrentIndex(X_WEEKLY);
                break;
            case X_YEARLY:
                startDate.setDate(x[xIndex], 1, 1);
                endDate.setDate(x[xIndex], 12, 31);
                ui->comboBox_x->setCurrentIndex(X_MONTHLY);
            }
        }
        plotGraph();
    }
}

void MainWindow::mousePlot(QCPAbstractPlottable *plottable,
                           QMouseEvent *event) {
    double key, value;
    int xIndex;
    QCPRange range;
    QDateTime date3;
    QDate date2;

    //    qDebug("Mouse button %08X",event->button());

    if(event->button() & Qt::LeftButton) {
        plottable->pixelsToCoords(event->x(), event->y(), key, value);
        xIndex=round(key);

        double val=y[xIndex];
        if (y2[xIndex] != -1)
            val +=y2[xIndex];
        date3=QDateTime::fromTime_t(x[xIndex]);
        QString statusMsg = "Maximum value ";
        if (ui->comboBox_y->currentIndex() == Y_WALKING_TIME) {
            statusMsg+=dayHoursMins(val);
        } else {
            statusMsg+=QString::number(val, 'f', 0);
        }

        if (ui->comboBox_x->currentIndex() == X_DAILY) {
            statusMsg += " on " + date3.toString("ddd d-MMM-yyyy");
        } else {
            statusMsg += ". Number of days =" + QString::number(z[xIndex], 'f', 0) + ". Average per day = ";

            if (ui->comboBox_y->currentIndex() == Y_WALKING_TIME) {
                statusMsg+=dayHoursMins(val/z[xIndex]);
            } else {
                statusMsg+=QString::number(val/z[xIndex], 'f', 0);
            }

            if (ui->comboBox_x->currentIndex() == X_WEEKLY) {
                statusMsg += ". From " + date3.toString("d-MMM-yyyy") + " to ";
                date3 = date3.addDays(6);
                statusMsg += date3.toString("d-MMM-yyyy");
            }
        }

        statusBar()->showMessage(statusMsg, 10000);
    }

    if((event->button() & Qt::RightButton) && (ui->comboBox_x->currentIndex() == X_DAILY)) {
        plottable->pixelsToCoords(event->x(), event->y(), key, value);
        xIndex=round(key);

        QString note;

        noteBinaryDate=QDateTime::fromTime_t(x[xIndex]);
        noteDate = noteBinaryDate.toString("yyyy-MM-dd");
        note = getDBNote(noteDate);
        if (note.size() == 0) {
            QMenu myMenu;
            QAction *action;

            action = new QAction("&Add Note", this);
            action->setToolTip("Adds a note to this day");
            connect(action, SIGNAL(triggered()), this, SLOT(addNote()));
            myMenu.addAction(action);

            myMenu.exec(event->globalPos());
        } else {
            editNote(1);
        }
    }
}

void MainWindow::addNote(void) {
    editNote(0);
}

void MainWindow::editNote(int type) {

    noteDialog *diag=new noteDialog(type, NULL, notePos, noteSize);
    QString title=noteBinaryDate.toString("'Notes for' ddd d-MMM-yyyy");

    diag->setWindowTitle(title);
    diag->setNote(getDBNote(noteDate));
    int msgBox_result=diag->exec();

    diag->getSizePos(noteSize, notePos);

    if (msgBox_result == diag->noteAccept)
        MainWindow::updDBNote(diag->getNote());
    if (msgBox_result == diag->noteDelete)
        MainWindow::updDBNote("");
}


void MainWindow::delNote()
{
    updDBNote("");
}

/*
 ** There is no formulea for converting walking speed & weight into calories.
 ** It appears to be a non-linear parabolic function. The easiest thing to do
 ** seems to be use a lookup table. The following table came from
 ** http://walk.walgreens.com/blog/post/calories-burned
 ** and has been converted to metric.
 ** The calories burnt is just a guesstimate. It varies on a whole host of
 ** variables that we don't know, such as age, sex, metabolic rate etc.
 */
static float speed [7] = {3.22, 4.02, 4.83, 5.63, 6.44, 7.24, 8.05};
static float weights [10] = {45.45, 54.55, 63.64, 72.73, 81.82, 90.91, 100.00, 113.64, 125.00, 136.36};
static float calories[7][10] = {{34.80, 42.25, 49.71, 56.54, 63.38, 70.84, 77.67, 88.23, 96.93, 105.63},
                                {34.18, 40.39, 47.22, 54.06, 60.89, 67.73, 74.56, 84.51, 93.21, 101.90},
                                {32.93, 39.77, 45.98, 52.82, 59.03, 65.87, 72.70, 82.64, 90.72, 98.80},
                                {32.31, 38.53, 45.36, 51.57, 58.41, 64.62, 70.84, 80.78, 88.86, 96.93},
                                {35.42, 42.25, 49.71, 56.54, 63.38, 70.84, 77.67, 88.23, 96.93, 105.63},
                                {39.77, 47.22, 55.30, 63.38, 71.46, 78.91, 86.99, 98.80, 108.74, 118.68},
                                {45.36, 54.06, 63.38, 72.08, 81.40, 90.10, 99.42, 113.09, 124.27, 135.46}};

int getSpeedIndex(float userSpeed) {
    unsigned int i;

    for (i=0; i<sizeof(speed)-1; i++) {
        if (userSpeed <= speed[i])
            return i;

        if (userSpeed > (speed[i] - ((speed[i+1] - speed[i])/2)) && userSpeed <= (speed[i] + ((speed[i+1] - speed[i])/2)))
            return i;
    }
    return sizeof(speed)-1;
}

int getWeightIndex(float userWeight) {
    unsigned int i;

    for (i=0; i<sizeof(weights)-1; i++) {
        if (userWeight <= weights[i])
            return i;

        if (userWeight > (weights[i] - ((weights[i+1] - weights[i])/2)) && userWeight <= (weights[i] + (weights[i+1] - weights[i])/2))
            return i;
    }
    return sizeof(weights)-1;
}

void MainWindow::recalibrate() {
    /*
     ** This routine is used to recalculate the walked distance based on a
     ** new step size, and kcal burnt based on weight/speed walked.
     ** Seems to me the best way to get a step size, it to get a long
     ** straight road. Measure the distance accurately on an map - use
     ** Google as a last resort; and the walk it and see how many steps
     ** it is.
     ** Plug this new step size into this dialog and it'll update all the
     ** walked distances in the database.
     */
    int msgBox_result;
    int i,j;

    QMessageBox msgBox( this->windowTitle()+tr(": Confirmation"),
                        tr("Do you really want to recalculate the kCal and distance walked in the database by adjusting the step size and/or weight ?"),
                        QMessageBox::Question,
                        QMessageBox::Yes,
                        QMessageBox::No,
                        QMessageBox::Escape,
                        this);

    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.setIcon(QMessageBox::Question);

    msgBox_result=msgBox.exec();

    if (msgBox_result == 3) {
        if (weight== -1 || stepSize == -1) {

            msgBox.setText("It appears as if the pedometer has never been connected. The program cannot continue until the existing step size and weight in the pedometer have been retrieved. Click 'OK' and download data from the pedometer before continuing.");
            msgBox.setIcon(QMessageBox::Information);
            msgBox.removeButton(msgBox.button(QMessageBox::No));
            msgBox.removeButton(msgBox.button(QMessageBox::Yes));
            msgBox.addButton(QMessageBox::Ok);
            msgBox_result=msgBox.exec();

            return;
        }
        RecalculateDialog *diag=new RecalculateDialog(NULL, recalcPos, recalcSize);
        diag->setMode(metric);
        diag->setStepSize(stepSize);
        diag->setWeight(weight);
        diag->setDate(minDate, maxDate);
        msgBox_result=diag->exec();

        diag->getSizePos(recalcSize, recalcPos);

        if (msgBox_result == QDialog::Accepted) {
            QSqlQuery query;
            QDate fromDate, toDate;
            int added, updated;
            int newStep = diag->getStepSize();
            int newWeight = diag->getWeight();

            diag->getDate(fromDate, toDate);

            /*
             ** If we are not metric, then simply convert the new step length &
             ** weight to metric and carry on as normal doing all calcs in metric
             */
            if (!metric) {
                newStep=round(newStep * 2.54);
                newWeight=round(newWeight * 0.453592);
            }
            query.prepare("select * from log where steps <> 0 and date >= ? and date <= ?");
            query.addBindValue(fromDate);
            query.addBindValue(toDate);

            if (!query.exec()) {
                qDebug("error executing: %s", qPrintable(query.lastError ().text() ) );
                return;
            }

            added=updated=0;
            while (query.next()) {

                QDateTime date=query.value(0).toDateTime();
                int steps=query.value(1).toInt();
                int steps_target=query.value(2).toInt();
                float stepDistance = query.value(3).toFloat();
                float weight = query.value(4).toFloat();
                float distance = query.value(5).toFloat();
                float newCalories = query.value(6).toFloat();
                float walkingTime = query.value(7).toFloat();

                stepDistance = newStep;
                distance = (newStep * steps)/100000.0;        // New Distance walked in km

                weight = newWeight;
                float velocity = distance/(walkingTime/60.0);
                i=getSpeedIndex(velocity);
                j=getWeightIndex(newWeight);

                newCalories = calories[i][j] * distance;
                insertUpdate(date.toString("yyyy-MM-dd"), steps, steps_target, stepDistance, weight, distance, newCalories, walkingTime, added, updated);
            }

            updStatus(added, updated);
            plotGraph();
        }
    }
}

void MainWindow::about() {
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("About PedometerLog 0.3.0");
    msgBox.setTextFormat(Qt::RichText);
    msgBox.setText("This program provides basic logging of data for an USB pedometer.<br>"
                   "The project home page can be found <a href='https://sourceforge.net/projects/pedometerlog/'>here.</a><br>"
                   "<br>Improvements are being constantly made."
                   "<br><br>If there is a change you'd like, but can't do yourself, try emailing me via sourceforge.net"
                   );
    msgBox.exec();
}

void MainWindow::updStatus(int added, int updated){

    QString status="";

    if (added > 1) {
        status="Added " + QString::number(added) + " records. ";
    }
    if (added == 1)
        status="Added 1 record. ";
    if (updated > 1) {
        status+="Updated " + QString::number(updated) + " records.";
    }
    if (updated == 1)
        status+="Updated 1 record.";

    statusBar()->showMessage(status, 10000);
}

void MainWindow::config() {
    configDialog *diag=new configDialog(NULL, configPos, configSize);
    diag->setColours(QColor::fromRgb(mainR, mainG, mainB, mainOpacity),
                     QColor::fromRgb(peakR, peakG, peakB, peakOpacity));

    diag->setHideMissing(hideMissingDays);
    diag->setDataMine(dataMine);
    if (diag->exec() == 1) {

        QColor main, peak;
        diag->getColours(main, peak);
        main.getRgb(&mainR, &mainG, &mainB, &mainOpacity);
        peak.getRgb(&peakR, &peakG, &peakB, &peakOpacity);
        hideMissingDays=diag->getHideMissing();
        dataMine=diag->getDataMine();
        plotGraph();
    }
    diag->getSizePos(configSize, configPos);
}

int MainWindow::xLabelInit(QCustomPlot *plot, int maxX) {
    /*
      When we have lots of bars on the graph, the labels on the X Axis tend to
      run into each other. It doesn't seem possible for the QCustomPlot
      code to automagically know this-> This is my attempt to calculate font
      sizes, and widths and decimate the labels.

      plot is a pointer to ui->customPlot
      maxX is the number of items we're trying to show on the x axis
      */

    QFont f=plot->xAxis->labelFont();
    QFontMetrics fm=QFontMetrics(f);
    //    qDebug() << "Font height=" << QString::number(fm.lineSpacing()) << "maxX=" << QString::number(maxX) << "Plot size=" << QString::number(plot->width());
    QSize s=plot->size();
    //    qDebug() << "Plot size=" << QString::number(s.width());

    double x=round(1.0/(s.width()/((fm.lineSpacing() * 1.5) * maxX)));
    if (x == 0) x++;
    return(x);
}

bool MainWindow::xLabelNeeded(QCustomPlot *plot, int noOfLines, int maxX) {
    /*
      This routine is called for every label we want to output. It calculates if
      it'll run into an adjacent label.

      plot is a pointer to ui->customPlot
      index is the current index on the x axis
      nooflines is the number of lines in the label (1 or 2)
      returns true if we think it's OK for this label.
      */

    if (xLabelCount == 0) {
        xLabelCount = xLabelInit(plot, (maxX + 1)*noOfLines);
        //        qDebug() << "Label count=" << QString::number(xLabelCount);
    }
    xLabelCount--;

    return (xLabelCount == 0);
}
