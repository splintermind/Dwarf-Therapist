/*
Dwarf Therapist
Copyright (c) 2010 Justin Ehlert

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
#ifndef SUPERLABOR_H
#define SUPERLABOR_H

#include <QObject>
#include "customprofession.h"
#include "dwarftherapist.h"
#include "laborlistbase.h"

namespace Ui
{
    class SuperLaborEditor;
}

class SuperLabor : public LaborListBase  {
    Q_OBJECT
public:
    SuperLabor(QObject *parent = 0);
    SuperLabor(CustomProfession *cp, QObject *parent = 0);
    SuperLabor(QSettings &s, QObject *parent = 0);

    QString get_name(){return m_name;}
    QString get_custom_prof_name(){return m_custom_prof_name;}
    QVector<int> get_enabled_labors();
    bool is_from_custom_prof(){return m_auto_generated;}

    int show_builder_dialog(QWidget *parent);
    void delete_from_disk();
    void save(QSettings &s);

public slots:
    void role_changed(int);

private:
    Ui::SuperLaborEditor *ui;    
    QString m_custom_prof_name;
    bool m_auto_generated; //indicates that it's really just a wrapper for a custom profession        
    bool is_valid();

};
#endif // SUPERLABOR_H
