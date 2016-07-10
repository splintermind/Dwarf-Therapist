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
#ifndef LANGUAGES_H
#define LANGUAGES_H

#include <QObject>
#include "utils.h"
#include "word.h"

class DFInstance;
class MemoryLayout;

class Languages : public QObject {
    Q_OBJECT
public:
    Languages(DFInstance *df, QObject *parent = 0);
    virtual ~Languages();

    static Languages* get_languages(DFInstance *df);

    QString language_word(VIRTADDR address);
    QString english_word(VIRTADDR address);

private:
    VIRTADDR m_address;
    QVector<Word *> m_language;
    QHash<int, QStringList> m_words;
    qint16 m_offset_lang;
    qint16 m_offset_words;
    qint16 m_offset_word_type;

    DFInstance * m_df;
    MemoryLayout * m_mem;

    void load_data();
    QStringList read_words(VIRTADDR addr, int lang_id = -1);

    QString word_chunk(uint word, int language_id);
    QString word_chunk_declined(uint word, Word::WORD_FORM w_form);

};

#endif // LANGUAGES_H
