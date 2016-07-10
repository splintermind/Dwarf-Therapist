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
#include "languages.h"
#include "dfinstance.h"
#include "memorylayout.h"
#include "truncatingfilelogger.h"

#include <QStringBuilder>

Languages::Languages(DFInstance *df, QObject *parent)
    : QObject(parent)
    , m_address(0)
    , m_df(df)
    , m_mem(df->memory_layout())
{
    load_data();
}

Languages::~Languages() {
    qDeleteAll(m_language);
    m_language.clear();
    m_words.clear();
    m_mem = 0;
    m_df = 0;
}

Languages* Languages::get_languages(DFInstance *df) {
    return new Languages(df);
}

void Languages::load_data() {
    if (!m_df || !m_df->memory_layout() || !m_df->memory_layout()->is_valid()) {
        LOGW << "load of Languages called but we're not connected";
        return;
    }
    // make sure our reference is up to date to the active memory layout
    m_mem = m_df->memory_layout();
    m_offset_lang = m_mem->word_offset("language_id");
    m_offset_words = m_mem->word_offset("words");
    m_offset_word_type =m_mem->word_offset("word_type");

    TRACE << "Starting refresh of Language data at" << hexify(m_address);

    LOGD << "Loading language translation tables";
    qDeleteAll(m_language);
    m_language.clear();
    m_words.clear();

    VIRTADDR generic_lang_table = m_mem->address("language_vector");
    VIRTADDR translation_vector = m_mem->address("translation_vector");
    VIRTADDR word_table_offset = m_mem->language_offset("word_table");
    TRACE << "LANGUAGES VECTOR" << hexify(translation_vector);
    TRACE << "GENERIC LANGUAGE VECTOR" << hexify(generic_lang_table);
    TRACE << "WORD TABLE OFFSET" << hexify(word_table_offset);

    m_df->attach();
    if (generic_lang_table != 0xFFFFFFFF && generic_lang_table != 0) {
        LOGD << "Loading generic strings from" << hexify(generic_lang_table);
        QVector<VIRTADDR> generic_words = m_df->enumerate_vector(generic_lang_table);
        LOGD << "generic words" << generic_words.size();
        foreach(VIRTADDR word_ptr, generic_words) {
            m_language << Word::get_word(m_df, word_ptr);
        }
    }
    if (translation_vector != 0xFFFFFFFF && translation_vector != 0) {
        QVector<VIRTADDR> languages = m_df->enumerate_vector(translation_vector);
        int id = 0;
        foreach(VIRTADDR lang, languages) {
            QString race_name = m_df->read_string(lang);
            TRACE << "FOUND LANG ENTRY" << hex << lang << race_name;
            VIRTADDR lang_table = lang + word_table_offset;
            TRACE << "Loading " << race_name << " strings from" << hex << lang_table;
            QVector<VIRTADDR> lang_words = m_df->enumerate_vector(lang_table);
            TRACE << race_name << " words" << lang_words.size();
            QStringList words_list;
            foreach(VIRTADDR word_ptr, lang_words) {
                words_list.append(m_df->read_string(word_ptr));
            }
            m_words.insert(id, words_list);
            id++;
        }
    }
    m_df->detach();
}

QString Languages::language_word(VIRTADDR addr)
{
    // front_compound, rear_compound, first_adjective, second_adjective, hyphen_compound
    // the_x, of_x
    int language_id = m_df->read_int(addr + m_offset_lang); //language_name.language
    QStringList words = read_words(addr,language_id);

    QStringList name_parts;
    name_parts << words.at(0) % words.at(1);
    if (!words.value(5).isEmpty())
    {
        name_parts << words.at(2) % words.at(3) % words.at(4) % words.at(5);
    }
    name_parts << words.at(6);

    return capitalizeEach(name_parts.join(" ").simplified());
}

QString Languages::english_word(VIRTADDR addr)
{
    // front_compound, rear_compound, first_adjective, second_adjective, hyphen_compound
    // the_x, of_x
    QStringList words = read_words(addr);
    QStringList name_parts;
    name_parts << words.at(0) % words.at(1);
    if (!words.value(5).isEmpty())
    {
        name_parts << "The" << words.at(2) << words.at(3);
        if (words.value(4).isEmpty())
            name_parts << words.at(5);
        else
        {
            name_parts << words.at(4) % "-" % capitalize(words.at(5));
        }
    }
    if (!words.value(6).isEmpty()){
        name_parts << "of" << words.at(6);
    }

    return capitalizeEach(name_parts.join(" ").simplified());
}

QStringList Languages::read_words(VIRTADDR addr, int lang_id){
    QStringList words;
    for (int i=0; i< 7; i++){
        if (lang_id < 0){
            Word::WORD_FORM w_form = static_cast<Word::WORD_FORM>(
                        m_df->read_short(addr + m_offset_word_type + i*sizeof(qint16)));
            //words id to lookup based on the word type
            words << word_chunk_declined(m_df->read_int(addr + m_offset_words + i*sizeof(qint32)), w_form);
        }else{
            words << word_chunk(m_df->read_int(addr + m_offset_words + i*sizeof(qint32)), lang_id);
        }
    }
    return words;
}

QString Languages::word_chunk_declined(uint word, Word::WORD_FORM w_form) {
    QString out = "";
    if (word != 0xFFFFFFFF) {
        out=m_language[word]->get_form(w_form);
    }
    return out;
}

QString Languages::word_chunk(uint word, int language_id)
{
    QString out = "";
    //if the language doesn't exist, use the last language (DF behaviour)
    if (!m_words.contains(language_id))
        language_id = m_words.count()-1;

    if (word != 0xFFFFFFFF)
        out = m_words.value(language_id).at(word);

    return out;
}
