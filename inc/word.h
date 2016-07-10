/*
Dwarf Therapist
Copyright (c) 2010 Justin Ehlert

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
#ifndef WORD_H
#define WORD_H

#include <QObject>
#include "utils.h"

class Dwarf;
class DFInstance;
class MemoryLayout;

class Word : public QObject {
    Q_OBJECT
public:
    Word(DFInstance *df, VIRTADDR address, QObject *parent = 0);
    virtual ~Word();

    static Word* get_word(DFInstance *df, const VIRTADDR &address);

    //! Return the memory address (in hex) of this creature in the remote DF process
    VIRTADDR address() {return m_address;}

    typedef enum {
        WF_NOUN,
        WF_NOUN_PL,
        WF_ADJ,
        WF_PRE,
        WF_VERB,
        WF_VERB_PRES,
        WF_VERB_PST,
        WF_VERB_PPP,
        WF_VERB_PRESP,
        WF_MAX
    } WORD_FORM;

    QString get_form(const WORD_FORM w_form);

    void refresh_data();

private:
    VIRTADDR m_address;
    QStringList m_forms;

    DFInstance * m_df;
    MemoryLayout * m_mem;

    void read_forms();
};

#endif
