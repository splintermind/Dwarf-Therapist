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
#ifndef DWARF_MODEL_PROXY_H
#define DWARF_MODEL_PROXY_H

#include <QJSEngine>
#include <QtWidgets>

class Dwarf;
class DwarfModel;

class DwarfModelProxy: public QSortFilterProxyModel {
    Q_OBJECT
public:
    //these roles are for the right click sorting of the name column
    typedef enum {
        DSR_NAME_ASC = 0,
        DSR_NAME_DESC,
        DSR_ID_ASC,
        DSR_ID_DESC,
        DSR_AGE_ASC,
        DSR_AGE_DESC,
        DSR_SIZE_ASC,
        DSR_SIZE_DESC,
        DSR_DEFAULT
    } DWARF_SORT_ROLE;

    DwarfModelProxy(QObject *parent = 0);
    DwarfModel* get_dwarf_model() const;
    void sort(int column, Qt::SortOrder order);    
    Qt::SortOrder m_last_sort_order;
    DWARF_SORT_ROLE m_last_sort_role;

    QList<QString> get_script_names() {return m_scripts.keys();}
    QString get_script(const QString script_name) {return m_scripts.value(script_name);}
    void clear_script(const QString script_name = "");

    void refresh_script();
    QList<Dwarf*> get_filtered_dwarves();       

public slots:
    void cell_activated(const QModelIndex &idx);    
    void setFilterFixedString(const QString &pattern);
    void sort(int, DwarfModelProxy::DWARF_SORT_ROLE, Qt::SortOrder order);
    void apply_script(const QString &script_name, const QString &script_body);
    void test_script(const QString &script_body);
    void clear_test();

signals:
    void filter_changed();

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
	bool filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const;
private:
	QString m_filter_text;
    QString m_test_script;
    QJSEngine *m_engine;
    QHash<QString,QString> m_scripts;
};

#endif
