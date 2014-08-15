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
#ifndef GRID_VIEW_DOCK_H
#define GRID_VIEW_DOCK_H

#include <QListWidgetItem>
#include "basedock.h"

class ViewManager;

namespace Ui {
	class GridViewDock;
}

class GridViewDock : public BaseDock {
	Q_OBJECT
public:
	GridViewDock(ViewManager *mgr, QWidget *parent = 0, Qt::WindowFlags flags = 0);
	void draw_views();

	public slots:
		void add_new_view();
		void draw_list_context_menu(const QPoint &pos);

private:
	ViewManager *m_manager;
	Ui::GridViewDock *ui;
	QListWidgetItem *m_tmp_item;

    short current_view_is_custom();

	private slots:
		void edit_view();
		void edit_view(QListWidgetItem*);
		void copy_view();
		void delete_view();
        void item_clicked(QListWidgetItem*);
};

#endif
