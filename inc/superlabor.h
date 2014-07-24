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

class SuperLabor : public QObject  {
    Q_OBJECT
public:
    SuperLabor(CustomProfession *cp, QObject *parent = 0)
        :QObject(parent)
    {
        m_name = cp->get_name();
        m_role_name = "";
        m_custom_prof_name = m_name;
        m_auto_generated = true;
    }

    SuperLabor(QSettings &s, QObject *parent = 0)
        :QObject(parent)
        , m_name(s.value("id","").toString())
        , m_role_name(s.value("role_name","").toString())
        , m_custom_prof_name(s.value("custom_prof_name","").toString())
        , m_auto_generated(false)
    {
        int labors = s.beginReadArray("labors");
        for (int i = 0; i < labors; ++i) {
            s.setArrayIndex(i);
            int labor = s.value("id", -1).toInt();
            if (labor != -1)
                m_labors << labor;
        }
        s.endArray();
    }

    SuperLabor()
    {
        m_role_name = "";
    }

    QString get_name(){return m_name;}
    QString get_custom_prof_name(){return m_custom_prof_name;}

    QVector<int> get_labors(){
        if(!m_custom_prof_name.isEmpty()){
            CustomProfession *cp = DT->get_custom_profession(m_custom_prof_name);
            if(cp)
                return cp->get_enabled_labors();
            else
                return m_labors;
        }else{
            return m_labors;
        }
    }

private:
    QString m_name;
    QString m_role_name;
    QString m_custom_prof_name;
    QVector<int> m_labors;
    bool m_auto_generated;

};
#endif // SUPERLABOR_H
