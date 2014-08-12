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

#ifndef ROLE_H
#define ROLE_H

#include <QObject>
#include <QSettings>
#include <QVector>
#include "math.h"

class Preference;
class RoleAspect;

class Role : public QObject {
    Q_OBJECT
public:
    Role();
    Role(QSettings &s, QObject *parent = 0);
    Role(const Role &r);
    virtual ~Role();

    QString name;
    QString script;
    bool is_custom;

    struct global_weight{
        bool is_default;
        float weight;
    };

    struct simple_rating{
        bool is_custom;
        float rating;
        QString name;
    };

    //unfortunately we need to keep all the keys as a string and cast them so we can use the same functions
    //ie can't pass in a hash with <string, aspect> and <int, aspect>
    QHash<QString, RoleAspect*> attributes;
    QHash<QString, RoleAspect*> skills;
    QHash<QString, RoleAspect*> traits;
    QVector<Preference*> prefs;

    //global weights
    global_weight attributes_weight;
    global_weight skills_weight;
    global_weight traits_weight;
    global_weight prefs_weight;

    QString get_role_details();

    void set_labors(QList<int> list){m_labors = list;}
    QList<int> get_labors() {return m_labors;}

    void create_role_details(QSettings &s);

    void write_to_ini(QSettings &s, float default_attributes_weight, float default_traits_weight, float default_skills_weight, float default_prefs_weight);

    Preference* has_preference(QString name);
protected:
    void parseAspect(QSettings &s, QString node, global_weight &g_weight, QHash<QString, RoleAspect *> &list, float default_weight);
    void parsePreferences(QSettings &s, QString node, global_weight &g_weight, float default_weight);
    void write_aspect_group(QSettings &s, QString group_name, global_weight group_weight, float group_default_weight, QHash<QString, RoleAspect *> &list);
    void write_pref_group(QSettings &s, float default_prefs_weight);

    QString get_aspect_details(QString title, global_weight aspect_group_weight, float aspect_default_weight, QHash<QString, RoleAspect *> &list);
    QString get_preference_details(float aspect_default_weight);

    QString generate_details(QString title, global_weight aspect_group_weight,float aspect_default_weight, QMap<QString,global_weight> list);

    QString role_details;
    QList<int> m_labors; //labors associated via the skills in the role
};
#endif // ROLE_H
