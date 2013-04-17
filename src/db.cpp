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
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QDate>

#include <QDebug>
#include <time.h>

QString reportSQLError(QSqlQuery query)
{
    QString result;

    result=qPrintable(query.lastError ().text());
    qDebug("error executing: %s. %s. Error (%d)", qPrintable(query.lastQuery()), qPrintable(result), query.lastError().number() );

    return result;
}


bool MainWindow::createConnection()
{
    db = QSqlDatabase::addDatabase("QMYSQL");
    db.setDatabaseName("PedometerLog");
    db.setConnectOptions();

    if (!db.open()) {
        qDebug() << db.lastError().text();

        QMessageBox::critical(0, tr("Cannot open database"),
                              tr("Unable to establish a database connection.\n"
                                 "This program needs MYSQL support.\n\n"
                                 "Click Cancel to exit."), QMessageBox::Cancel);
        return false;
    }
    return true;
}


bool MainWindow::makeNotes(void) {
    /*
      This routine is run everytime the program starts up, but will only ever do something
      once. That something is to create a notes table so that notes can be saved and displayed
      against individual days. This feature is added in version 0.2.0
      */

    QSqlQuery query;

    query.prepare("SELECT * FROM notes");
    if (!query.exec()) {
        if (query.lastError().number() != 1146) {
            reportSQLError(query);
            return false;
        } else {
            query.prepare("create table if not exists notes (date date not null, note text)");

            if (!query.exec()) {
                reportSQLError(query);
                return false;
            }

            query.prepare("create unique index notes_idx on notes (date);");
            if (!query.exec()) {
                reportSQLError(query);
                return false;
            }
        }
    }
    return(true);
}

bool MainWindow::insertUpdate(QString date, int steps, int targetStep, float stepDistance,
                              float weight, float distance, float calories, float walkingTime,
                              int &added, int &updated)
{
    QSqlQuery query;
    QDate now = QDate::currentDate();

    QString format = "yyyy-MM-dd";
    QString buffer= now.toString(format);

    query.prepare("SELECT * FROM log where date=?");
    query.addBindValue(date);
    if (!query.exec()) {
        reportSQLError(query);
        return false;
    }

    /*
     ** If not metric convert relevant fields to metric
     */
    if (!metric) {
        stepDistance = stepDistance * 2.54;
        weight = weight *0.45359;
        distance = distance * 1.6099344;
    }

    if (query.next()) {
        QString date=query.value(0).toString();
        int dbSteps=query.value(1).toInt();
        int dbsteps_target=query.value(2).toInt();
        float dbstepDistance = query.value(3).toFloat();
        float dbweight = query.value(4).toFloat();
        float dbdistance = query.value(5).toFloat();
        float dbcalories = query.value(6).toFloat();
        float dbwalkingTime = query.value(7).toFloat();

        if (dbSteps < steps ||
            (dbSteps == steps &&
             (dbsteps_target != targetStep ||
              dbstepDistance != stepDistance ||
              dbweight != weight ||
              dbdistance != distance ||
              dbcalories != calories ||
              dbwalkingTime != walkingTime))) {

            query.prepare("update log set "
                          "steps = :steps, target = :target, stepSize = :stepSize, "
                          "weight = :weight, km = :km, kcal = :kcal, walkingtime=:walktime "
                          "where date = :key");

            query.bindValue(":steps", steps);
            query.bindValue(":target", targetStep);
            query.bindValue(":kcal", calories);
            query.bindValue(":walktime", walkingTime);
            query.bindValue(":key", date);
            query.bindValue(":stepSize", stepDistance);
            query.bindValue(":weight", weight);
            query.bindValue(":km", distance);

            updated++;          
        }
    } else {
        // Record with date does not exist, so create a record.

        query.prepare("insert into log (date, steps,"
                      "target, stepSize, weight, km, kcal, walkingtime)"
                      "values (?, ?, ?, ?, ?, ?, ?, ?)");

        query.addBindValue(date);
        query.addBindValue(steps);
        query.addBindValue(targetStep);
        query.addBindValue(stepDistance);
        query.addBindValue(weight);
        query.addBindValue(distance);
        query.addBindValue(calories);
        query.addBindValue(walkingTime);

        added++;
    }

    if (!query.exec())
    {
        reportSQLError(query);
        return false;
    }

    return true;
}


QString MainWindow::getDbStatus() {

    QSqlQuery query;
    QString result;
    result.resize(0);

    query.prepare("SELECT max(date) as max FROM log;");
    if (!query.exec())
    {
        return reportSQLError(query);
    }

    QDate now = QDate::currentDate();
    query.next();
    if (query.isNull(0)) {
        result = "Database is empty.";
        return result;
    } else {
        QDate maxdate = query.value(0).toDate();

        if (maxdate.daysTo(now) !=0) {
            if (maxdate.daysTo(now) == 1) {
                result = "There is 1 days worth of data missing from database.";
            } else {
                result = "There are ";
                result += QString::number(maxdate.daysTo(now));
                result.append(" days missing from database");
            }
        }
    }

    return result;
}

QString MainWindow:: getDBNote(QString date) {
    QSqlQuery query;
    QString result;
    result.resize(0);

    query.prepare("SELECT note from notes where date = :key");
    query.bindValue(":key", date);

    if (!query.exec())
    {
        reportSQLError(query);
        return result;
    }

    if (query.next()) {
        return query.value(0).toString();
    }
    return "";
}

void MainWindow::updDBNote(QString note) {
    QSqlQuery query;

    if (note.size() == 0) {
        /*
          Note size is zero, so delete any note on file
          */
        query.prepare("delete from notes where date = :key");
        query.bindValue(":key", noteDate);
        if (!query.exec()) {
            reportSQLError(query);
        }
    } else {
        query.prepare("SELECT note from notes where date = :key");
        query.bindValue(":key", noteDate);

        if (!query.exec()) {
            reportSQLError(query);
        }

        if (query.next()) {
            /*
              Record exists, so update it
              */
            query.prepare("update notes set note=:value where date=:key");
        } else {
            /*
              Record does not exist so add it
              */
            query.prepare("insert into notes values (:key, :value)");
        }
        query.bindValue(":key", noteDate);
        query.bindValue(":value", note);
        if (!query.exec()) {
            reportSQLError(query);
        }
    }
}
