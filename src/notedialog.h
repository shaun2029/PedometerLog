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

#ifndef NOTEDIALOG_H
#define NOTEDIALOG_H

#include <QDialog>

namespace Ui {
    class noteDialog;
}

class noteDialog : public QDialog
{
    Q_OBJECT

public:
    explicit noteDialog(int type, QWidget *parent = 0, QPoint point=QPoint(200,200), QSize size=QSize(100,100));
    ~noteDialog();

    void getSizePos(QSize&, QPoint&);
    QString getNote(void);
    void setNote(QString);

    enum noteClick { noteAccept=100, noteReject, noteDelete };

private:
    Ui::noteDialog *ui;

private slots:
    void accept();
    void reject();
    void deleteClicked();
};

#endif // NOTEDIALOG_H
