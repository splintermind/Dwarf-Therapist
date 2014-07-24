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

#include "superlaborcolumn.h"
#include "columntypes.h"
#include "viewcolumnset.h"
#include "gamedatareader.h"
#include "dwarfmodel.h"
#include "truncatingfilelogger.h"
#include "superlabor.h"
#include "dwarftherapist.h"
#include "dwarf.h"
#include "labor.h"

SuperLaborColumn::SuperLaborColumn(const QString &title, QString id, ViewColumnSet *set, QObject *parent)
    : ViewColumn(title, CT_SUPER_LABOR, set, parent)
    , m_id(id)
{
}

SuperLaborColumn::SuperLaborColumn(QSettings &s, ViewColumnSet *set, QObject *parent)
    : ViewColumn(s, set, parent)
    , m_id(s.value("id").toString())
{
}

SuperLaborColumn::SuperLaborColumn(const SuperLaborColumn &to_copy)
    : ViewColumn(to_copy)
    , m_id(to_copy.m_id)
{
}

QStandardItem *SuperLaborColumn::build_cell(Dwarf *d) {
    QStandardItem *item = init_cell(d);
    SuperLabor *sl = DT->get_super_labor(m_id);

    float rating = 0.0;
    QList<QVariant> related_labors;
    QStringList labor_names;

    foreach(int labor_id, sl->get_labors()){
        Labor *l = GameDataReader::ptr()->get_labor(labor_id);
        QString name = l->name;
        if(d->labor_enabled(l->labor_id)){
            labor_names.append("<b>" + name + "</b>");
        }else{
            labor_names.append(name);
        }
        related_labors.append(labor_id);
        rating += d->skill_level(GameDataReader::ptr()->get_labor(labor_id)->skill_id,false,true);
    }
    rating /= sl->get_labors().count();

    item->setData(sl->get_custom_prof_name(), DwarfModel::DR_LABOR_ID);
    item->setData(rating, DwarfModel::DR_RATING);
    item->setData(rating, DwarfModel::DR_SORT_VALUE);
    item->setData(rating, DwarfModel::DR_DISPLAY_RATING);
    item->setData(CT_SUPER_LABOR, DwarfModel::DR_COL_TYPE);
    item->setData(related_labors,DwarfModel::DR_SPECIAL_FLAG);

    QString labors_desc = "";
    labors_desc = QString("<br/><br/><b>Associated Labors:</b> %1").arg(labor_names.count() <= 0 ? "None" : labor_names.join(", "));
    QString prof_desc = "";
    if(!sl->get_custom_prof_name().isEmpty())
        prof_desc = tr("<br/><b>Custom Profession:</b> %1").arg(sl->get_custom_prof_name());

    QString tooltip = QString("<center><h3>%1</h3></center><b>Average Skill:</b> %2%3%4%5")
            .arg(m_id)
            .arg(QString::number(rating,'f',2))
            .arg(prof_desc)
            .arg(labors_desc)
            .arg(tooltip_name_footer(d));

    item->setToolTip(tooltip);

    return item;
}

QStandardItem *SuperLaborColumn::build_aggregate(const QString &group_name, const QVector<Dwarf*> &dwarves){
    Q_UNUSED(dwarves);
    QStandardItem *item = init_aggregate(group_name);
    return item;
}

void SuperLaborColumn::write_to_ini(QSettings &s){
    ViewColumn::write_to_ini(s);
    s.setValue("id",m_id);
}

