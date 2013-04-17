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
#include "configdialog.h"
#include "ui_configdialog.h"

#include <QColorDialog>
#include <QWidget>
#include <QDebug>

configDialog::configDialog(QWidget *parent, QPoint pos, QSize size) :
    QDialog(parent),
    ui(new Ui::configDialog)
{
    ui->setupUi(this);

    resize(size);
    move(pos);

    connect(ui->mainColourPushbutton, SIGNAL(clicked()), SLOT(mainColorPushbuttonClicked()));
    connect(ui->peakColourPushbutton, SIGNAL(clicked()), SLOT(peakColorPushbuttonClicked()));

    connect(ui->pushButtonOK, SIGNAL(clicked()), SLOT(apply()));
    connect(ui->pushButtonCancel, SIGNAL(clicked()), SLOT(reject()));
}

configDialog::~configDialog()
{
    delete ui;
}

void configDialog::apply() {
    QPoint p=pos();

    done(QDialog::Accepted);
}

void configDialog::reject() {
    QPoint p=pos();

    done(QDialog::Rejected);
}

void configDialog::setColours(QColor main, QColor peak) {
    int r,g,b,opacity;
    QString style;

    mainGraph=main;
    peakGraph=peak;

    main.getRgb(&r, &g, &b, &opacity);
    ui->mainColourPushbutton->setWindowOpacity(opacity);

    style="background-color: rgba(" + QString::number(r) + ","
            + QString::number(g) + ","
            + QString::number(b) + ","
            + QString::number(opacity)
            + "); color: rgb(0,0,0,)";
    ui->mainColourPushbutton->setStyleSheet(style);

    peak.getRgb(&r, &g, &b, &opacity);
    ui->peakColourPushbutton->setWindowOpacity(opacity);

    style="background-color: rgba(" + QString::number(r) + ","
            + QString::number(g) + ","
            + QString::number(b) + ","
            + QString::number(opacity)
            + "); color: rgb(0,0,0,)";
    ui->peakColourPushbutton->setStyleSheet(style);
}


void configDialog::getColours(QColor &main, QColor &peak) {
    main=mainGraph;
    peak=peakGraph;
}

void configDialog::getSizePos(QSize &size, QPoint &position) {
    position=this->window()->pos();
    size=this->window()->size();
}

void chooseColour(QColor &c) {

    QColorDialog *dialog= new QColorDialog(c);
    dialog->setOptions(QColorDialog::ShowAlphaChannel);
    dialog->setWindowTitle("Select Colour");
    if (dialog->exec() == 1){

        c=dialog->selectedColor();
    }
}

void configDialog::mainColorPushbuttonClicked() {
    int r,g,b,opacity;
    QString style;

    chooseColour(mainGraph);
    mainGraph.getRgb(&r, &g, &b, &opacity);
    ui->mainColourPushbutton->setWindowOpacity(opacity);

    style="background-color: rgba(" + QString::number(r) + ","
            + QString::number(g) + ","
            + QString::number(b) + ","
            + QString::number(opacity)
            + "); color: rgb(0,0,0,)";
    ui->mainColourPushbutton->setStyleSheet(style);
}

void configDialog::peakColorPushbuttonClicked() {
    int r,g,b,opacity;
    QString style;

    chooseColour(peakGraph);
    peakGraph.getRgb(&r, &g, &b, &opacity);
    ui->peakColourPushbutton->setWindowOpacity(opacity);

    style="background-color: rgba(" + QString::number(r) + ","
            + QString::number(g) + ","
            + QString::number(b) + ","
            + QString::number(opacity)
            + "); color: rgb(0,0,0,)";
    ui->peakColourPushbutton->setStyleSheet(style);
}

void configDialog::setHideMissing(bool param) {
    ui->hideMissing->setChecked(param);
}

bool configDialog::getHideMissing(void) {
    return(ui->hideMissing->checkState());
}

void configDialog::setDataMine(bool param) {
    ui->dataMine->setChecked(param);
}

bool configDialog::getDataMine(void) {
    return(ui->dataMine->checkState());
}
