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
#include "notedialog.h"
#include "ui_notedialog.h"

#include <QColorDialog>
#include <QPushButton>
#include <QWidget>
#include <QDebug>

noteDialog::noteDialog(int type, QWidget *parent, QPoint pos, QSize size) :
    QDialog(parent),
    ui(new Ui::noteDialog)
{
    ui->setupUi(this);

    resize(size);
    move(pos);

    ui->buttonBox->addButton(tr("OK"), QDialogButtonBox::AcceptRole);

    ui->buttonBox->addButton(tr("Close without Saving"),QDialogButtonBox::RejectRole);

    if (type){
        QPushButton *button;

        button = new QPushButton( "Delete Note" );
        connect( button, SIGNAL(clicked()), this, SLOT(deleteClicked()) );
        ui->buttonBox->addButton( button, QDialogButtonBox::DestructiveRole );
    }
}

noteDialog::~noteDialog()
{
    delete ui;
}

void noteDialog::getSizePos(QSize &size, QPoint &position) {
    position=this->window()->pos();
    size=this->window()->size();
}

void noteDialog::setNote(QString str) {
    ui->textEdit->setText(str);
}

QString noteDialog::getNote() {
    return(ui->textEdit->toPlainText());
}

void noteDialog::accept() {
    QPoint p=pos();

    done(QDialog::Accepted);
    noteDialog::setResult(noteAccept);
}

void noteDialog::reject() {
    QPoint p=pos();

    done(QDialog::Rejected);
    noteDialog::setResult(noteReject);
}

void noteDialog::deleteClicked() {
    QPoint p=pos();
    done(QDialog::Rejected);
    noteDialog::setResult(noteDelete);
}
