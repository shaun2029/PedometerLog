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
#include "recalculatedialog.h"
#include "ui_recalculatedialog.h"
#include <QDebug>
#include <QErrorMessage>
#include <QMessageBox>

RecalculateDialog::RecalculateDialog(QWidget *parent, QPoint pos, QSize size) :
        QDialog(parent),
        ui(new Ui::RecalculateDialog)
{
    ui->setupUi(this);

    resize(size);
    move(pos);

    ui->pushButtonApply->setEnabled(false);

    connect(ui->pushButtonApply, SIGNAL(clicked()), SLOT(apply()));
    connect(ui->pushButtonCancel, SIGNAL(clicked()), SLOT(reject()));
    connect(ui->newStepSize, SIGNAL(editingFinished()), SLOT(checkStepSize()));
    connect(ui->newWeight, SIGNAL(editingFinished()), SLOT(checkWeight()));
}

RecalculateDialog::~RecalculateDialog() {
    delete ui;
}

void RecalculateDialog::setMode(bool m) {

    metric=m;
    if (metric) {
        ui->sizeUnitsLabel->setText("cm");
        ui->weightUnitsLabel->setText("kg");
    } else {
        ui->sizeUnitsLabel->setText("inches");
        ui->weightUnitsLabel->setText("lbs.");
    }
}

void RecalculateDialog::setStepSize(int x) {
    QString str;

    stepSize=x;
    str="Current step size " + QString::number(x);
    if (metric) {
        str+="cm";
        stepHi=213; stepLo=30;
    } else {
        str+="inches";
        stepHi=84; stepLo=12;
    }
    ui->currentSizeLabel->setText(str);
    ui->newStepSize->setText(QString::number(x));
}

void RecalculateDialog::setWeight(int x) {
    QString str;

    weight=x;
    str="Current weight " + QString::number(x);
    if (metric) {
        str+="kg";
        weightLo=20; weightHi=227;
    } else {
        str+="lbs";
        weightLo=50; weightHi=500;
    }
    ui->currentWeightLabel->setText(str);
    ui->newWeight->setText(QString::number(x));
}

void RecalculateDialog::setDate(QDate fromDate, QDate toDate) {
    ui->fromDate->setDateRange(fromDate, toDate);
    ui->fromDate->setDate(fromDate);
    ui->toDate->setDateRange(fromDate, toDate);
    ui->toDate->setDate(toDate);
}

int RecalculateDialog::getStepSize() {
    return(stepSize);
}

int RecalculateDialog::getWeight() {
    return(weight);
}

void RecalculateDialog::getSizePos(QSize &size, QPoint &position) {
    position=this->window()->pos();
    size=this->window()->size();
}

void RecalculateDialog::checkStepSize() {
    bool ok;
    int value = ui->newStepSize->text().toInt(&ok);
    if (!ok || value < stepLo || value > stepHi) {
        QString str = "Step size must be between " + QString::number(stepLo) + " and " + QString::number(stepHi);
        if (metric)
            str += " cm.";
        else
            str += " inches.";

        QMessageBox errorMessage( this->windowTitle()+tr(": Error"),
                                  str,
                                  QMessageBox::Warning,
                                  QMessageBox::Ok,
                                  QMessageBox::Abort,
                                  QMessageBox::Accepted);

        errorMessage.removeButton(errorMessage.button(QMessageBox::Abort));
        errorMessage.removeButton(errorMessage.button(QMessageBox::Ok));
        errorMessage.exec();
        ui->newStepSize->setFocus();
    } else
        ui->pushButtonApply->setEnabled(true);
}

void RecalculateDialog::checkWeight() {
    bool ok;
    int value = ui->newWeight->text().toInt(&ok);
    if (!ok || value < weightLo || value > weightHi) {
        QString str = "Weight must be between " + QString::number(weightLo) + " and " + QString::number(weightHi);
        if (metric)
            str += " kg.";
        else
            str += " lbs.";

        QMessageBox errorMessage( this->windowTitle()+tr(": Error"),
                                  str,
                                  QMessageBox::Warning,
                                  QMessageBox::Ok,
                                  QMessageBox::Abort,
                                  QMessageBox::Accepted);

        errorMessage.removeButton(errorMessage.button(QMessageBox::Abort));
        errorMessage.removeButton(errorMessage.button(QMessageBox::Ok));
        errorMessage.exec();

        ui->newWeight->setFocus();
    } else
        ui->pushButtonApply->setEnabled(true);

}

void RecalculateDialog::apply() {
    QPoint p=pos();

    done(QDialog::Accepted);
}

void RecalculateDialog::reject() {
    QPoint p=pos();

    done(QDialog::Rejected);
}


void RecalculateDialog::getDate(QDate &from, QDate &to) {
    from=fromDate=ui->fromDate->date();
    to=toDate=ui->toDate->date();
}
