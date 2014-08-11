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
#ifndef CUSTOMPROFESSIONCOLUMN_H
#define CUSTOMPROFESSIONCOLUMN_H

#include "viewcolumn.h"
#include "superlaborcolumn.h"
#include "global_enums.h"
#include "multilabor.h"

class ViewColumn;
class Dwarf;
class Attribute;

class CustomProfessionColumn : public SuperLaborColumn {
    Q_OBJECT
public:
    CustomProfessionColumn(const QString &title, QString id, ViewColumnSet *set = 0, QObject *parent = 0);
    CustomProfessionColumn(QSettings &s, ViewColumnSet *set = 0, QObject *parent = 0);
    CustomProfessionColumn(const CustomProfessionColumn &to_copy); // copy ctor
    CustomProfessionColumn* clone() {return new CustomProfessionColumn(*this);}
    QStandardItem *build_cell(Dwarf *d);
    QStandardItem *build_aggregate(const QString &group_name, const QVector<Dwarf*> &dwarves);
protected:
    MultiLabor* get_base_object();
private:
    void init();
};
#endif // CUSTOMPROFESSIONCOLUMN_H
