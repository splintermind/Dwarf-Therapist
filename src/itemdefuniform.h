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
#ifndef ITEMDEFUNIFORM_H
#define ITEMDEFUNIFORM_H

#include <QObject>
#include "global_enums.h"
#include "utils.h"

class DFInstance;
class MemoryLayout;

class ItemDefUniform : public QObject {
    Q_OBJECT
public:
    ItemDefUniform(DFInstance *df, VIRTADDR address, QObject *parent = 0);

    ItemDefUniform(ITEM_TYPE itype, int item_id, QObject *parent = 0);

    ItemDefUniform(ITEM_TYPE itype, short sub_type, short job_skill, QObject *parent = 0);

    ItemDefUniform(const ItemDefUniform &uItem);

    virtual ~ItemDefUniform();

    VIRTADDR address() {return m_address;}
    ITEM_TYPE item_type() const {return m_iType;}
    void item_type(ITEM_TYPE newType){m_iType = newType;}
    short item_subtype() {return m_subType;}
    short mat_type() {return m_matType;}
    int mat_index() {return m_mat_index;}
    MATERIAL_CLASS mat_class(){return m_mat_class;}
    int id(){return m_id;}
    bool indv_choice(){return m_indv_choice;}
    short job_skill(){return m_job_skill;}

    int get_stack_size(){return m_stack_size;}
    void add_to_stack(int num){m_stack_size+=num;}

private:
    VIRTADDR m_address;
    DFInstance * m_df;
    MemoryLayout * m_mem;
    ITEM_TYPE m_iType;
    short m_subType;
    short m_matType;
    int m_mat_index;
    MATERIAL_CLASS m_mat_class;
    int m_id;
    bool m_indv_choice;
    short m_job_skill;
    int m_stack_size;

    void read_data();
};

#endif // ITEMDEFUNIFORM_H
