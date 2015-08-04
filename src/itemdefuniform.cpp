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

#include "itemdefuniform.h"
#include "dfinstance.h"
#include "memorylayout.h"

ItemDefUniform::ItemDefUniform(DFInstance *df, VIRTADDR address, QObject *parent)
    : QObject(parent)
    , m_address(address)
    , m_df(df)
    , m_mem(df->memory_layout())
    , m_iType(NONE)
    , m_subType(-1)
    , m_matType(-1)
    , m_mat_index(-1)
    , m_mat_class(MC_NONE)
    , m_id(-1)
    , m_indv_choice(false)
    , m_job_skill(-1)
    , m_stack_size(1)
{
    read_data();
}

ItemDefUniform::ItemDefUniform(ITEM_TYPE itype, int item_id, QObject *parent)
    : QObject(parent)
    , m_address(0x0)
    , m_df(0x0)
    , m_mem(0x0)
    , m_iType(itype)
    , m_subType(-1)
    , m_matType(-1)
    , m_mat_index(-1)
    , m_mat_class(MC_NONE)
    , m_id(item_id)
    , m_indv_choice(false)
    , m_job_skill(-1)
    , m_stack_size(1)
{}

ItemDefUniform::ItemDefUniform(ITEM_TYPE itype, short sub_type, short job_skill, QObject *parent)
    : QObject(parent)
    , m_address(0x0)
    , m_df(0x0)
    , m_mem(0x0)
    , m_iType(itype)
    , m_subType(sub_type)
    , m_matType(-1)
    , m_mat_index(-1)
    , m_mat_class(MC_NONE)
    , m_id(-1)
    , m_indv_choice(false)
    , m_job_skill(job_skill)
    , m_stack_size(1)
{}

ItemDefUniform::ItemDefUniform(const ItemDefUniform &uItem)
    : QObject(uItem.parent())
    , m_address(uItem.m_address)
    , m_df(uItem.m_df)
    , m_mem(uItem.m_mem)
    , m_iType(uItem.m_iType)
    , m_subType(uItem.m_subType)
    , m_matType(uItem.m_matType)
    , m_mat_index(uItem.m_mat_index)
    , m_mat_class(uItem.m_mat_class)
    , m_id(uItem.m_id)
    , m_indv_choice(uItem.m_indv_choice)
    , m_job_skill(uItem.m_job_skill)
    , m_stack_size(uItem.m_stack_size)
{}

ItemDefUniform::~ItemDefUniform(){
    m_df = 0;
    m_mem = 0;
}

void ItemDefUniform::read_data(){
    if(m_address > 0){
        m_id = m_df->read_int(m_address);

        VIRTADDR uniform_addr = m_address + m_df->memory_layout()->squad_offset("uniform_item_filter"); //filter offset start
        m_iType = static_cast<ITEM_TYPE>(m_df->read_short(uniform_addr));
        m_subType = m_df->read_short(uniform_addr + m_df->memory_layout()->item_filter_offset("item_subtype"));
        m_mat_class = static_cast<MATERIAL_CLASS>(m_df->read_short(uniform_addr + m_df->memory_layout()->item_filter_offset("mat_class")));
        m_matType = m_df->read_short(uniform_addr + m_df->memory_layout()->item_filter_offset("mat_type"));
        m_mat_index = m_df->read_int(uniform_addr + m_df->memory_layout()->item_filter_offset("mat_index"));
        //individual choice is stored in a bit array, first bit (any) second (melee) third (ranged)
        //currently we only care if one is set or not. it may be ok just to check for a weapon type as well
        m_indv_choice = m_df->read_addr(m_address + m_df->memory_layout()->squad_offset("uniform_indiv_choice")) & 0x7;
    }
}
