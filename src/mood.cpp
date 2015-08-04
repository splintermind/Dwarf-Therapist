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

#include "mood.h"
#include "utils.h"
#include <QSettings>

Mood::Mood(QObject *parent)
    : QObject(parent)
    , m_name("")
    , m_name_colored("")
    , m_desc("")
    , m_desc_colored("")
    , m_color(QColor(Qt::black)) {}

Mood::Mood(QSettings &s, QObject *parent)
    : QObject(parent)
{
    m_name = s.value("name","").toString();
    m_desc = capitalize(s.value("description","").toString());
    m_color = QColor(s.value("color","#000000").toString());

    m_name_colored = QString("<font color=%1>%2</font>").arg(m_color.name()).arg(m_name);
    m_desc_colored = QString("<font color=%1>%2</font>").arg(m_color.name()).arg(m_desc);
}
