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
#ifndef BELIEF_H
#define BELIEF_H

#include <QtWidgets>
#include "global_enums.h"
#include "dwarfstats.h"
#include "gamedatareader.h"
#include "fortressentity.h"

class Belief : public QObject {
    Q_OBJECT

private:
    //! this map will hold the minimum_value -> string (e.g. level 76-90 of ANXIETY_PROPENSITY is "Is always tense and jittery")
    QMap<int, QString> m_level_string;

public:
    Belief(int id, QSettings &s, QObject *parent = 0);

    QString name;
    int m_id;

    int belief_id(){return m_id;}
    bool is_active(const short &personal_val);
    QString level_message(const short &val);
};

#endif
