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
#ifndef RECALCULATEDIALOG_H
#define RECALCULATEDIALOG_H

#include <QDialog>
#include <QDate>

namespace Ui {
    class RecalculateDialog;
}

class RecalculateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RecalculateDialog(QWidget *parent = 0,QPoint point=QPoint(200,200), QSize size=QSize(100,100));
    ~RecalculateDialog();

    void setMode(bool);
    void setStepSize(int);
    void setWeight(int);
    void setDate(QDate, QDate);

    int getStepSize();
    int getWeight();
    void getDate(QDate&, QDate&);
    void getSizePos(QSize&, QPoint&);

private slots:
    void checkStepSize();
    void checkWeight();
    void apply();
    void reject();
private:
    Ui::RecalculateDialog *ui;
    bool metric;
    int stepSize;
    int weight;
    QDate fromDate, toDate;
    int stepHi, stepLo;
    int weightHi, weightLo;
};

#endif // RECALCULATEDIALOG_H
