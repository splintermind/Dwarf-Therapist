/*
Dwarf Therapist
Copyright (c) 2009 Trey Stout (chmod)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#ifndef PREFERENCES_DOCK_H
#define PREFERENCES_DOCK_H

#include <QTableWidget>
#include <QRegExp>
#include "basedock.h"

class PreferencesDock : public BaseDock {
    Q_OBJECT
public:
    PreferencesDock(QWidget *parent = 0, Qt::WindowFlags flags = 0);

    void clear();
    void refresh();
    void filter();

protected:
    QTableWidget *tw_prefs;
    QRegExp m_filter;

public slots:
    //void cell_clicked(int r,int c);
    void clear_filter();
    void clear_search();
    void search_changed(QString);
    void selection_changed();

signals:
    void item_selected(QList<QPair<QString,QString> >);

private:
    void closeEvent(QCloseEvent *event);

};

#endif // PREFERENCES_DOCK_H

