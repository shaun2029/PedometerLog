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
#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDialog>

namespace Ui {
    class configDialog;
}

class configDialog : public QDialog
{
    Q_OBJECT

public:
    explicit configDialog(QWidget *parent=0, QPoint point=QPoint(200,200), QSize size=QSize(100,100));
    ~configDialog();

    void setColours(QColor, QColor);
    void getColours(QColor&, QColor&);
    void getSizePos(QSize&, QPoint&);
    bool getHideMissing(void);
    void setHideMissing(bool);
    bool getDataMine(void);
    void setDataMine(bool);

private slots:

    void apply();
    void reject();
    void mainColorPushbuttonClicked();
    void peakColorPushbuttonClicked();
private:
    Ui::configDialog *ui;

    QColor mainGraph;
    QColor peakGraph;
};

#endif // CONFIGDIALOG_H
