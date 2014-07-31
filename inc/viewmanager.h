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

#ifndef VIEW_MANAGER_H
#define VIEW_MANAGER_H

#include <QtWidgets>
#include "columntypes.h"
#include "viewcolumn.h"

class Dwarf;
class GridView;
class ViewColumnSet;
class StateTableView;
class DwarfModel;
class DwarfModelProxy;

/*!
 * This class is an un-enforced singleton that holds a collection of all configured GridViews
 * It also serves as the main view of the app (the tab widget) It creates many to one mappings
 * of gridviews to visible tabs. Each visible tab is an instance of StateTableView
 */
class ViewManager : public QTabWidget {
	Q_OBJECT
public:
	ViewManager(DwarfModel *dm, DwarfModelProxy *proxy, QWidget *parent = 0);
    virtual ~ViewManager();
    QList<Dwarf*> get_selected_dwarfs() {return m_selected_dwarfs;}
    
	QList<GridView*> views() {return m_views;}
	void add_view(GridView *view);
    void add_weapons_view(QList<GridView*> &built_in_views);

    static void save_column_sort(COLUMN_TYPE cType, ViewColumn::COLUMN_SORT_TYPE sType);
    static ViewColumn::COLUMN_SORT_TYPE get_default_col_sort(COLUMN_TYPE cType){
        return m_default_column_sort.value(cType,ViewColumn::CST_DEFAULT);
    }

	public slots:
        void setCurrentIndex(int idx);
		void reload_views();
		void write_views();
		void draw_views();
        void write_tab_settings();
		void set_group_by(int group_by);
		void redraw_current_tab();
        void redraw_current_tab_headers();
        void redraw_specific_header(int id, COLUMN_TYPE type);

		GridView *get_view(const QString &name);
		GridView *get_active_view();
		void remove_view(GridView *view);
		void replace_view(GridView *old_view, GridView *new_view);

		// passthru
		void expand_all();
		void collapse_all();
		void jump_to_dwarf(QTreeWidgetItem* current, QTreeWidgetItem* previous);
        void jump_to_profession(QTreeWidgetItem* current, QTreeWidgetItem* previous);

        void select_all();

        void clear_selected();
        void reselect(QVector<int> ids);

        void refresh_custom_professions();
        void rebuild_global_sort_keys();

private:
	QList<GridView*> m_views;
	DwarfModel *m_model;
	DwarfModelProxy *m_proxy;
	QToolButton *m_add_tab_button;
    QList<Dwarf*> m_selected_dwarfs;    
    int m_last_index;
    QErrorMessage *m_squad_warning;
    bool m_reset_sorting;

    StateTableView *get_stv(int idx = -1);    

    static QMap<COLUMN_TYPE, ViewColumn::COLUMN_SORT_TYPE> m_default_column_sort;

	private slots:
		//! used when adding tabs via the tool button
		int add_tab_from_action();
		//! used from everywhere else
		int add_tab_for_gridview(GridView *v);
		void remove_tab_for_gridview(int index);
		void draw_add_tab_button();
        void dwarf_selection_changed(const QItemSelection &selected, const QItemSelection &deselected);
        void show_squad_warning();

signals:
	void dwarf_focus_changed(Dwarf *d);    
    void group_changed(const int);
    void selection_changed();
		
};

#endif
