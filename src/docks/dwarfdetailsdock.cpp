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
#include <QSplitter>
#include "basedock.h"
#include "dwarfdetailsdock.h"
#include "dwarfdetailswidget.h"
#include "dwarf.h"

DwarfDetailsDock::DwarfDetailsDock(QWidget *parent, Qt::WindowFlags flags)
    : BaseDock(parent, flags)
    , m_widget(new DwarfDetailsWidget(this))
    , m_initialized(false)
    , m_current_id(-1)
{
    m_widget->hide();
    setWindowTitle(tr("Dwarf Details"));
    setObjectName("dock_dwarf_details");
    setFeatures(QDockWidget::AllDockWidgetFeatures);
    setAllowedAreas(Qt::AllDockWidgetAreas);
    lbl_info = new QLabel(tr("Click on a dwarf name to show details here"), this);
    lbl_info->setAlignment(Qt::AlignCenter);
    setWidget(lbl_info);
}

void DwarfDetailsDock::show_dwarf(Dwarf *d) {
    m_widget->show_dwarf(d);
    if (!m_initialized) {
        m_initialized = true;
        setWidget(m_widget);
        m_widget->show();
    }  
    m_current_id = d->id();
}

QByteArray DwarfDetailsDock::splitter_sizes(){
    QSplitter* split = m_widget->findChild<QSplitter *>("details_splitter");
    if(split)
        return split->saveState();
    else
        return NULL;
}

void DwarfDetailsDock::clear(bool reinit){
    m_widget->clear();
    m_current_id = -1;
    if(reinit){
        m_initialized = false;
        setWidget(lbl_info);
    }
}
