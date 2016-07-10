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
#include "word.h"
#include "dfinstance.h"
#include "memorylayout.h"
#include "dwarftherapist.h"
#include "mainwindow.h"
#include "truncatingfilelogger.h"

Word::Word(DFInstance *df, VIRTADDR address, QObject *parent)
    : QObject(parent)
    , m_address(address)
    , m_df(df)
    , m_mem(df->memory_layout())
{
    refresh_data();
}

Word::~Word() {
}

Word* Word::get_word(DFInstance *df, const VIRTADDR & address) {
    return new Word(df, address);
}

void Word::refresh_data() {
    if (!m_df || !m_df->memory_layout() || !m_df->memory_layout()->is_valid()) {
        LOGW << "refresh of Word called but we're not connected";
        return;
    }
    // make sure our reference is up to date to the active memory layout
    m_mem = m_df->memory_layout();
    TRACE << "Starting refresh of Word data at" << hexify(m_address);

    read_forms();
}

void Word::read_forms() {
    m_forms.clear();
    //m_forms << m_df->read_string(m_address + m_mem->word_offset("base"));
    TRACE << "read word " << m_forms.value(0,"UNK");
    m_forms << m_df->read_string(m_address + m_mem->word_offset("noun_singular"));
    m_forms << m_df->read_string(m_address + m_mem->word_offset("noun_plural"));
    m_forms << m_df->read_string(m_address + m_mem->word_offset("adjective"));
    m_forms << "";//m_df->read_string(m_address + m_mem->word_offset("prefix"));
    m_forms << m_df->read_string(m_address + m_mem->word_offset("verb"));
    m_forms << m_df->read_string(m_address + m_mem->word_offset("present_simple_verb"));
    m_forms << m_df->read_string(m_address + m_mem->word_offset("past_simple_verb"));
    m_forms << m_df->read_string(m_address + m_mem->word_offset("past_participle_verb"));
    m_forms << m_df->read_string(m_address + m_mem->word_offset("present_participle_verb"));
}

QString Word::get_form(const WORD_FORM w_form){
    return m_forms.value((int)w_form,"");
}
