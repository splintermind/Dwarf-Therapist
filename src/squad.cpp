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
#include "squad.h"
#include "dwarf.h"
#include "word.h"
#include "dwarfmodel.h"
#include "dfinstance.h"
#include "memorylayout.h"
#include "dwarftherapist.h"
#include "mainwindow.h"
#include "truncatingfilelogger.h"
#include "fortressentity.h"
#include "uniform.h"
#include "math.h"

Squad::Squad(int id, DFInstance *df, VIRTADDR address, QObject *parent)
    : QObject(parent)    
    , m_address(address)
    , m_id(id)
    , m_df(df)
    , m_mem(df->memory_layout())
{
    read_data();
}

Squad::~Squad() {
    m_df = 0;
    m_uniforms.clear();
}

Squad* Squad::get_squad(int id, DFInstance *df, const VIRTADDR & address) {
    return new Squad(id, df, address);
}

void Squad::read_data() {
    if (!m_df || !m_df->memory_layout() || !m_df->memory_layout()->is_valid()) {
        LOGW << "refresh of squad called but we're not connected";
        return;
    }
    // make sure our reference is up to date to the active memory layout
    m_mem = m_df->memory_layout();
    TRACE << "Starting refresh of squad data at" << hexify(m_address);

    //qDeleteAll(m_members);
    m_members.clear();

    if(m_id < 0)
        read_id();
    read_name();
    read_members();
    read_activities();
}

void Squad::read_id() {
    m_id = m_df->read_int(m_address + m_mem->squad_offset("id"));
    TRACE << "ID:" << m_id;
}

void Squad::read_name() {
    m_name = m_df->get_translated_word(m_address + m_mem->squad_offset("name"));
    QString alias = m_df->read_string(m_address + m_mem->squad_offset("alias"));
    if(alias != "")
        m_name = alias;
    TRACE << "Name:" << m_name;
    m_pending_name = m_name;
}


void Squad::read_members() {
    VIRTADDR addr;
    m_members_addr = m_df->enumerate_vector(m_address + m_mem->squad_offset("members"));
    int member_count = 0;
    foreach(addr, m_members_addr){
        if(m_df->read_int(addr)>0)
            member_count++;
    }

    short carry_food = m_df->read_short(m_address+m_mem->squad_offset("carry_food"));
    short carry_water = m_df->read_short(m_address+m_mem->squad_offset("carry_water"));
    int ammo_count = 0;
    foreach(addr, m_df->enumerate_vector(m_address+m_mem->squad_offset("ammunition"))){
        ammo_count += m_df->read_int(addr+0xc);//TODO: add proper offset
    }
    int ammo_each = 0;
    if(member_count > 0  && ammo_count > 0)
        ammo_each = ceil((float)ammo_count / member_count);

    //read the uniforms
    int position = 0;
    Uniform *u;    
    foreach(addr, m_members_addr){
        u = new Uniform(m_df,this);
        int hist_id = m_df->read_int(addr);
        m_members.insert(position,hist_id);
        read_equip_category(addr+m_mem->squad_offset("armor_vector"),ARMOR,u);
        read_equip_category(addr+m_mem->squad_offset("helm_vector"),HELM,u);
        read_equip_category(addr+m_mem->squad_offset("pants_vector"),PANTS,u);
        read_equip_category(addr+m_mem->squad_offset("gloves_vector"),GLOVES,u);
        read_equip_category(addr+m_mem->squad_offset("shoes_vector"),SHOES,u);
        read_equip_category(addr+m_mem->squad_offset("shield_vector"),SHIELD,u);
        read_equip_category(addr+m_mem->squad_offset("weapon_vector"),WEAPON,u);

        //add other items
        if(ammo_count > 0){
            u->add_uniform_item(addr+m_mem->squad_offset("quiver"),QUIVER);
            u->add_uniform_item(AMMO,-1,-1,ammo_each);
        }
        if(carry_food)
            u->add_uniform_item(addr+m_mem->squad_offset("backpack"),BACKPACK);
        if(carry_water)
            u->add_uniform_item(addr+m_mem->squad_offset("flask"),FLASK);

        m_uniforms.insert(position,u);
        position++;

        QVector<VIRTADDR> orders = m_df->enumerate_vector(addr+0x0004);
        foreach(VIRTADDR addr, orders){
            read_order(addr,hist_id);
        }
    }
}

void Squad::read_order(VIRTADDR addr, int hist_id){
    VIRTADDR vtable_addr = m_df->read_addr(addr);
    int ot = m_df->read_int(m_df->read_addr(vtable_addr+0xc)+0x1);
    ACT_ORDER_TYPE ord_type = ORDER_MOVE;
    if(ot > 0){
        ord_type = static_cast<ACT_ORDER_TYPE>(ot+(int)ORDER_MOVE);
    }
    if(ord_type != ORDER_TRAIN){
        if(hist_id >= 0){
            squad_activity sa;
            sa.act_type = ord_type;
            sa.skill_id = -1;
            m_unit_activities.insert(hist_id,sa);
        }else{
            m_squad_order = ord_type;
        }
    }
}

void Squad::read_activities(){
    QVector<VIRTADDR> orders_addrs = m_df->enumerate_vector(m_address + 0x009c);
    m_squad_order = ACT_NONE;
    foreach(VIRTADDR addr, orders_addrs){
        read_order(addr);
    }

    QVector<VIRTADDR> schedule_orders = m_df->enumerate_vector(m_address + 0xac);
    int offset_step = 0x0040;
    foreach(VIRTADDR sch_addr, schedule_orders){ //array of 12 structs, 0x40 in size
        VIRTADDR base_addr = sch_addr;
        for(int idx = 0; idx < 12; idx++){ //only check the current month's orders
            QVector<VIRTADDR> sch_ord_addrs = m_df->enumerate_vector(base_addr + 0x20);
            foreach(VIRTADDR sch_ord_addr, sch_ord_addrs){
                VIRTADDR ord_adr = m_df->read_addr(sch_ord_addr);
                read_order(ord_adr);
            }
            base_addr += offset_step;
        }
    }

    //if(m_squad_order == ACT_NONE){
        //read individual activities
        int m_activity = m_df->read_int(m_address + 0x00f4);
        QVector<VIRTADDR> all_activites_addrs = m_df->enumerate_vector(m_mem->get_base_addr() + 0x1ab3410);
        foreach(VIRTADDR addr, all_activites_addrs){
            int id = m_df->read_int(addr);
            if(id == m_activity){
                QVector<VIRTADDR> events = m_df->enumerate_vector(addr+0x0008);
                foreach(VIRTADDR evt_addr, events){
                    VIRTADDR vtable_addr = m_df->read_addr(evt_addr);
                    ACT_ORDER_TYPE evt_type = static_cast<ACT_ORDER_TYPE>(m_df->read_int(m_df->read_addr(vtable_addr)+0x1));
                    if(evt_type >= ACT_TRAINING && evt_type <= ACT_RANGED){
                        squad_activity sa_lead;
                        sa_lead.skill_id = -1;
                        if(evt_type == ACT_SKILL_DEMO){
                            int leader_id = m_df->read_int(evt_addr+0x0064);
                            sa_lead.skill_id = m_df->read_int(evt_addr+0x0068);
                            sa_lead.act_type = ACT_LEAD_DEMO;
                            m_unit_activities.insert(leader_id,sa_lead);
                        }
                        foreach(VIRTADDR hist_id, m_df->enumerate_vector(evt_addr+0x0014)){
                            if(evt_type > static_cast<squad_activity>(m_unit_activities.value(hist_id)).act_type){
                                squad_activity sa;
                                sa.skill_id = -1;
                                if(evt_type == ACT_SKILL_DEMO){
                                    sa.skill_id = sa_lead.skill_id;
                                    sa.act_type = ACT_WATCH_DEMO;
                                    m_unit_activities.insert(hist_id,sa);
                                }else{
                                    sa.act_type = evt_type;
                                    m_unit_activities.insert(hist_id,sa);
                                }
                            }
                        }
                    }
                }
                break;
            }
        }
    //}
}

QPair<ACT_ORDER_TYPE,QString> Squad::get_current_activity(int hist_id){
    QString activity;
    if(m_unit_activities.contains(hist_id)){
        squad_activity sa = m_unit_activities.value(hist_id);
        activity = get_activity_desc(static_cast<ACT_ORDER_TYPE>(sa.act_type));
        if(sa.skill_id >= 0){
            activity = activity.replace("??",GameDataReader::ptr()->get_skill_name(sa.skill_id));
        }
        return qMakePair(sa.act_type,activity);
    }else{
        return qMakePair(m_squad_order,get_activity_desc(m_squad_order));
    }
}


void Squad::read_equip_category(VIRTADDR vec_addr, ITEM_TYPE itype, Uniform *u){
    QVector<VIRTADDR> uniform_items = m_df->enumerate_vector(vec_addr);
    foreach(VIRTADDR uItem_addr, uniform_items){
        u->add_uniform_item(uItem_addr,itype,1);
    }
    //u->add_equip_count(itype,uniform_items.count());
}

int Squad::assigned_count(){
    int count = 0;
    foreach(int id, m_members.values()){
        if(id > 0)
            count++;
    }
    return count;
}

void Squad::assign_to_squad(Dwarf *d, bool committing){
    int assigned = assigned_count();

    //if committing, then the current squad is from the original id, otherwise use the pending id
    int current_squad_id = d->squad_id((committing ? true : false));

    //users could potentially select more than 10 and assign to squad
    if(d == 0 || assigned==10 || !d->is_adult() || current_squad_id == m_id)
        return;

    //if they already belong to a squad, remove them from the original squad
    if(current_squad_id >= 0){
        Squad *old = m_df->get_squad(current_squad_id);
        if(old){
            if(!old->remove_from_squad(d,committing)){
                //there was a problem removing them from the existing squad, don't continue
                return;
            }
        }
    }

    int position = -1;
    if(committing){
        //try to use the pending position already set first
        position = d->squad_position();
        //check the current id for the squad position we want
        int hist_id = m_members.value(position);
        if(hist_id != -1){
            //something has changed in the game, this position is no longer available. try to find an empty position
            position = find_position(-1);
        }
        VIRTADDR addr = 0;
        if(position >= 0){
            addr = m_members_addr.at(position);
        }
        if(addr > 0){
            m_df->write_int(addr,d->historical_id());
            m_df->write_int(d->address() + m_df->memory_layout()->dwarf_offset("squad_id"), m_id);
            m_df->write_int(d->address() + m_df->memory_layout()->dwarf_offset("squad_position"), position);
        }
    }else{
        position = find_position(-1); //find the first open position
        if(position >= 0){
            m_uniforms.value(position)->clear();
            d->update_squad_info(m_id,position,m_name);
        }
    }

    if(position >= 0)
        m_members.insert(position,d->historical_id());

    if(position == 0)
        emit squad_leader_changed();
}

bool Squad::remove_from_squad(Dwarf *d, bool committing){
    int position = find_position(d->historical_id());

    if(position >= 0){
        if(committing){
            VIRTADDR addr = m_members_addr.at(position);
            if(addr <= 0)
                return false;

            m_df->write_int(addr, -1);
            m_df->write_int(d->address() + m_df->memory_layout()->dwarf_offset("squad_id"), -1);
            m_df->write_int(d->address() + m_df->memory_layout()->dwarf_offset("squad_position"), -1);

        }else{
            m_uniforms.value(position)->clear();
            d->update_squad_info(-1,-1,"");
        }
        m_members.insert(position,-1);
    }

    if(position == 0)
        emit squad_leader_changed();

    return true;
}

int Squad::find_position(int hist_id){
    int position = -1;
    //find an open position
    foreach(int idx, m_members.uniqueKeys()){
        if(m_members.value(idx) == hist_id){
            position = idx;
            break;
        }
    }
    return position;
}

void Squad::rename_squad(QString alias){
    m_pending_name = alias;
}
void Squad::commit_pending(){
    if(m_name != m_pending_name){
        if(m_df->fortress()->squad_is_active(m_id)){
            m_df->write_string(m_address + m_df->memory_layout()->squad_offset("alias"),m_pending_name);
        }
    }
}
void Squad::clear_pending(){
    if(m_name != m_pending_name)
        m_pending_name = m_name;
}

int Squad::pending_changes(){
    return (m_name != m_pending_name ? 1 : 0);
}

QTreeWidgetItem *Squad::get_pending_changes_tree() {
    QTreeWidgetItem *s_item = new QTreeWidgetItem;
    s_item->setText(0, QString("%1 (%2)").arg(m_name).arg(pending_changes()));
    s_item->setData(0, Qt::UserRole, m_id);
    if (m_pending_name != m_name) {
        QTreeWidgetItem *i = new QTreeWidgetItem(s_item);
        i->setText(0, tr("Rename squad %1 to %2").arg(m_name).arg(m_pending_name));
        i->setIcon(0, QIcon(":img/book--pencil.png"));
        i->setToolTip(0, i->text(0));
        i->setData(0, Qt::UserRole, -1);
    }
    return s_item;
}
