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

#ifndef GRID_VIEW_H
#define GRID_VIEW_H

#include <QObject>
#include <QString>
#include <QStandardItemModel>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QSettings>

class ViewColumnSet;
class ViewColumn;
class ViewManager;
class GridViewDialog;

/*!
The idea: GridViews have many ViewColumnSets, which in turn have many 
ViewColumns. 

The ViewColumn has a data accessor to get at the underlying data for
a given row. 

ViewColumns make up a ViewColumnSet, which has a background color and
maintains the order of its member columns.

ViewColumnSets make up a GridView, which can be named and saved for 
use in other forts. This should allow maximum configurability for how
users may want their labor columns grouped, named, and colored.
*/

/*!
GridView
*/
class GridView : public QObject {
	Q_OBJECT
public:
    GridView(QString name, QObject *parent = 0);
	GridView(const GridView &to_be_copied); // copy ctor
	virtual ~GridView();

    void re_parent(QObject *parent);

	const QString name() const {return m_name;}
	void set_name(const QString &name) {m_name = name;}
    bool show_animals() const {return m_show_animals;}
    void set_show_animals(const bool show_animals) {m_show_animals = show_animals;}
	void add_set(ViewColumnSet *set);
	void remove_set(QString name);
	void remove_set(ViewColumnSet *set) {m_sets.removeAll(set);}
	void clear();
	const QList<ViewColumnSet*> sets() const {return m_sets;}
	bool is_active() {return m_active;}
	void set_active(bool active) {m_active = active;}
	ViewColumnSet *get_set(const QString &name);
    ViewColumn *get_column(const int idx);
	ViewColumnSet *get_set(int offset) {return m_sets.at(offset);}
	void set_is_custom(bool is_custom) {m_is_custom = is_custom;}
	bool is_custom() {return m_is_custom;}

	//! order of sets was changed by a view, so reflect those changes internally
	void reorder_sets(const QStandardItemModel &model);
	
	void write_to_ini(QSettings &settings);

	//! Factory function to create a gridview from a QSettings that has already been pointed at a gridview entry
    static GridView *read_from_ini(QSettings &settings, QObject *parent = 0);

    struct name_custom_sort
    {
        bool operator() (const GridView* g1, const GridView* g2) const
        {
//            if(g1->m_is_custom != g2->m_is_custom)
//                return ((int)g1->m_is_custom >= (int)g2->m_is_custom);
//            else
//                 return (g1->m_name < g2->m_name);

           return (g1->m_name < g2->m_name);
        }
    };

private:
	bool m_active;
	QString m_name;
	QList<ViewColumnSet*> m_sets;
	bool m_is_custom;
    bool m_show_animals;

signals:
    void updated(const GridView*);
};

#endif
