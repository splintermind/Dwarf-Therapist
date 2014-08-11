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
#ifndef SKILL_COLUMN_H
#define SKILL_COLUMN_H

#include "viewcolumn.h"
class Role;

class SkillColumn : public ViewColumn {
public:
    SkillColumn(const QString &title, const int &skill_id, ViewColumnSet *set = 0, QObject *parent = 0, COLUMN_TYPE cType = CT_SKILL);
    SkillColumn(QSettings &s, ViewColumnSet *set = 0, QObject *parent = 0);
    SkillColumn(const SkillColumn &to_copy); // copy ctor
    SkillColumn* clone() {return new SkillColumn(*this);}
	QStandardItem *build_cell(Dwarf *d);
	QStandardItem *build_aggregate(const QString &group_name, const QVector<Dwarf*> &dwarves);
	int skill_id() {return m_skill_id;}
	void set_skill_id(int skill_id) {m_skill_id = skill_id;}

	//override    
    void write_to_ini(QSettings &s);

public slots:    
    void refresh_sort(COLUMN_SORT_TYPE sType);

protected:
	int m_skill_id;
    float m_sort_val;
    void build_tooltip(Dwarf *d, bool include_roles);
    void refresh_sort(Dwarf *d, COLUMN_SORT_TYPE sType = CST_LEVEL);

    virtual float get_base_sort(Dwarf *d);
    virtual float get_role_rating(Dwarf *d);
    virtual float get_skill_rating(int id, Dwarf *d);
    virtual float get_skill_rate_rating(int id, Dwarf *d);

};

#endif
