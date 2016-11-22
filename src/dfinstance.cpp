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

#include "dfinstance.h"
#include "cp437codec.h"
#include "defines.h"
#include "dwarf.h"
#include "squad.h"
#include "word.h"
#include "gamedatareader.h"
#include "memorylayout.h"
#include "dwarftherapist.h"
#include "memorysegment.h"
#include "truncatingfilelogger.h"
#include "mainwindow.h"
#include "dwarfstats.h"
#include "languages.h"
#include "reaction.h"
#include "races.h"
#include "fortressentity.h"
#include "material.h"
#include "plant.h"
#include "item.h"
#include "itemweaponsubtype.h"
#include "itemarmorsubtype.h"
#include "itemgenericsubtype.h"
#include "itemtoolsubtype.h"
#include "preference.h"
#include "histfigure.h"
#include "emotiongroup.h"
#include <QMessageBox>
#include <QTimer>
#include <QTime>
#include <QByteArrayMatcher>

#include "caste.h"

#ifdef Q_OS_WIN
#define LAYOUT_SUBDIR "windows"
#include "dfinstancewindows.h"
#elif defined(Q_OS_LINUX)
#define LAYOUT_SUBDIR "linux"
#include "dfinstancelinux.h"
#elif defined(Q_OS_MAC)
#define LAYOUT_SUBDIR "osx"
#include "dfinstanceosx.h"
#endif

quint32 DFInstance::ticks_per_day = 1200;
quint32 DFInstance::ticks_per_month = 28 * DFInstance::ticks_per_day;
quint32 DFInstance::ticks_per_season = 3 * DFInstance::ticks_per_month;
quint32 DFInstance::ticks_per_year = 12 * DFInstance::ticks_per_month;

DFInstance::DFInstance(QObject* parent)
    : QObject(parent)
    , m_stop_scan(false)
    , m_is_ok(true)
    , m_bytes_scanned(0)
    , m_layout(0)
    , m_attach_count(0)
    , m_heartbeat_timer(new QTimer(this))
    , m_dwarf_race_id(0)
    , m_dwarf_civ_id(0)
    , m_current_year(0)
    , m_cur_year_tick(0)
    , m_cur_time(0)
    , m_languages(0x0)
    , m_fortress(0x0)
    , m_fortress_name(tr("Embarking"))
    , m_fortress_name_translated("")
    , m_squad_vector(0)
{
    // let subclasses start the heartbeat timer, since we don't want to be
    // checking before we're connected
    connect(m_heartbeat_timer, SIGNAL(timeout()), SLOT(heartbeat()));

    QDir d(QString("share:memory_layouts/%1").arg(LAYOUT_SUBDIR));
    d.setNameFilters(QStringList() << "*.ini");
    d.setFilter(QDir::NoDotAndDotDot | QDir::Readable | QDir::Files);
    d.setSorting(QDir::Name | QDir::Reversed);
    QFileInfoList files = d.entryInfoList();
    foreach(QFileInfo info, files) {
        MemoryLayout *temp = new MemoryLayout(info.absoluteFilePath());
        if (temp->is_valid()) {
            LOGI << "adding valid layout" << temp->game_version()
                 << temp->checksum();
            m_memory_layouts.insert(temp->checksum().toLower(), temp);
        } else {
            LOGI << "ignoring invalid layout" << info.absoluteFilePath();
            delete temp;
        }
    }

    // if no memory layouts were found that's a critical error
    if (m_memory_layouts.size() < 1) {
        LOGE << "No valid memory layouts found in the following directories..."
             << QDir::searchPaths("share");
        qApp->exit(ERROR_NO_VALID_LAYOUTS);
    }

    if (!QTextCodec::codecForName("IBM437"))
        // register CP437Codec so it can be accessed by name
        new CP437Codec();
}

DFInstance * DFInstance::newInstance(){
#ifdef Q_OS_WIN
    return new DFInstanceWindows();
#elif defined(Q_OS_MAC)
    return new DFInstanceOSX();
#elif defined(Q_OS_LINUX)
    return new DFInstanceLinux();
#endif
}

bool DFInstance::check_vector(const VIRTADDR start, const VIRTADDR end, const VIRTADDR addr){
    TRACE << "beginning vector enumeration at" << hex << addr;
    TRACE << "start of vector" << hex << start;
    TRACE << "end of vector" << hex << end;

    int entries = (end - start) / sizeof(VIRTADDR);
    TRACE << "there appears to be" << entries << "entries in this vector";

    bool is_acceptable_size = true;

    if (entries > 1000000) {
        LOGW << "vector at" << hexify(addr) << "has over 1.000.000 entries! (" << entries << ")";
        is_acceptable_size = false;
    }else if (entries > 250000){
        LOGW << "vector at" << hexify(addr) << "has over 250.000 entries! (" << entries << ")";
    }else if (entries > 50000){
        LOGW << "vector at" << hexify(addr) << "has over 50.000 entries! (" << entries << ")";
    }else if (entries > 10000){
        LOGW << "vector at" << hexify(addr) << "has over 10.000 entries! (" << entries << ")";
    }

    if(!is_acceptable_size){
        LOGE << "vector at" << hexify(addr) << "was not read due to an unacceptable size! (" << entries << ")";
    }

    return is_acceptable_size;
}

DFInstance::~DFInstance() {
    foreach(MemoryLayout *l, m_memory_layouts) {
        delete(l);
    }
    m_memory_layouts.clear();
    m_layout = 0;

    delete m_languages;
    delete m_fortress;

    qDeleteAll(m_inorganics_vector);
    m_inorganics_vector.clear();
    qDeleteAll(m_base_materials);
    m_base_materials.clear();

    qDeleteAll(m_reactions);
    m_reactions.clear();
    qDeleteAll(m_races);
    m_races.clear();

    m_ordered_weapon_defs.clear();
    foreach (const QList<ItemSubtype*> &list, m_item_subtypes) {
        foreach (ItemSubtype* def, list) {
            delete def;
        }
    }

    qDeleteAll(m_plants_vector);
    m_plants_vector.clear();

    qDeleteAll(m_pref_counts);
    m_pref_counts.clear();

    qDeleteAll(m_emotion_counts);
    m_emotion_counts.clear();

    m_equip_warning_counts.clear();
}

QVector<VIRTADDR> DFInstance::enumerate_vector(const VIRTADDR &addr) {
    return enum_vec<VIRTADDR>(addr);
}

QVector<qint16> DFInstance::enumerate_vector_short(const VIRTADDR &addr){
    return enum_vec<qint16>(addr);
}

template<typename T>
QVector<T> DFInstance::enum_vec(const VIRTADDR &addr){
    QVector<T> out;
    VIRTADDR start = read_addr(addr);
    VIRTADDR end = read_addr(addr + 4);
    USIZE bytes = end - start;
    if (check_vector(start,end,addr)){
        out.resize(bytes / sizeof(T));
        USIZE bytes_read = read_raw(start, bytes, out.data());
        TRACE << "FOUND" << bytes_read / sizeof(VIRTADDR) << "addresses in vector at" << hexify(addr);
    }
    return out;
}

USIZE DFInstance::read_raw(const VIRTADDR &addr, const USIZE &bytes, QByteArray &buffer) {
    buffer.resize(bytes);
    return read_raw(addr, bytes, buffer.data());
}

BYTE DFInstance::read_byte(const VIRTADDR &addr) {
    BYTE out;
    read_raw(addr, sizeof(BYTE), &out);
    return out;
}

WORD DFInstance::read_word(const VIRTADDR &addr) {
    WORD out;
    read_raw(addr, sizeof(WORD), &out);
    return out;
}

VIRTADDR DFInstance::read_addr(const VIRTADDR &addr) {
    VIRTADDR out;
    read_raw(addr, sizeof(VIRTADDR), &out);
    return out;
}

qint16 DFInstance::read_short(const VIRTADDR &addr) {
    qint16 out;
    read_raw(addr, sizeof(qint16), &out);
    return out;
}

qint32 DFInstance::read_int(const VIRTADDR &addr) {
    qint32 out;
    read_raw(addr, sizeof(qint32), &out);
    return out;
}

USIZE DFInstance::write_int(const VIRTADDR &addr, const int &val) {
    return write_raw(addr, sizeof(int), &val);
}

USIZE DFInstance::write_raw(const VIRTADDR &addr, const USIZE &bytes, const QByteArray &buffer) {
    return write_raw(addr, bytes, buffer.data());
}

void DFInstance::load_game_data()
{
    emit progress_message(tr("Loading languages"));
    if(m_languages){
        delete m_languages;
        m_languages = 0;
    }
    m_languages = Languages::get_languages(this);

    emit progress_message(tr("Loading reactions"));
    qDeleteAll(m_reactions);
    m_reactions.clear();
    load_reactions();

    emit progress_message(tr("Loading item and material lists"));
    qDeleteAll(m_plants_vector);
    m_plants_vector.clear();
    qDeleteAll(m_inorganics_vector);
    m_inorganics_vector.clear();
    qDeleteAll(m_base_materials);
    m_base_materials.clear();
    load_main_vectors();

    //load the currently played race before races and castes so we can load additional information for the current race being played
    VIRTADDR dwarf_race_index_addr = m_layout->address("dwarf_race_index");
    LOGD << "dwarf race index" << hexify(dwarf_race_index_addr);
    // which race id is dwarven?
    m_dwarf_race_id = read_short(dwarf_race_index_addr);
    LOGD << "dwarf race:" << hexify(m_dwarf_race_id);


    emit progress_message(tr("Loading races and castes"));
    qDeleteAll(m_races);
    m_races.clear();
    load_races_castes();

    emit progress_message(tr("Loading item types"));
    load_item_defs();

    load_fortress_name();
}

QString DFInstance::get_language_word(VIRTADDR addr){
    return m_languages->language_word(addr);
}

QString DFInstance::get_translated_word(VIRTADDR addr){
    return m_languages->english_word(addr);
}

QVector<Dwarf*> DFInstance::load_dwarves() {
    QVector<Dwarf*> dwarves;
    if (!m_is_ok) {
        LOGW << "not connected";
        detach();
        return dwarves;
    }

    // we're connected, make sure we have good addresses
    VIRTADDR creature_vector = m_layout->address("creature_vector");
    VIRTADDR current_year = m_layout->address("current_year");
    VIRTADDR current_year_tick = m_layout->address("cur_year_tick");
    m_cur_year_tick = read_int(current_year_tick);

    //current race's offset was bad
    if (!DT->arena_mode && m_dwarf_race_id < 0){
        return dwarves;
    }

    // both necessary addresses are valid, so let's try to read the creatures
    VIRTADDR dwarf_civ_idx_addr = m_layout->address("dwarf_civ_index");
    LOGD << "loading creatures from " << hexify(creature_vector);
    LOGD << "loading current year from" << hexify(current_year);

    emit progress_message(tr("Loading Units"));

    attach();
    m_dwarf_civ_id = read_int(dwarf_civ_idx_addr);
    LOGD << "civilization id:" << hexify(m_dwarf_civ_id);

    m_current_year = read_word(current_year);
    LOGI << "current year:" << m_current_year;
    m_cur_time = (int)m_current_year * 0x62700 + m_cur_year_tick;

    QVector<VIRTADDR> creatures_addrs = get_creatures();

    emit progress_range(0, creatures_addrs.size()-1);
    TRACE << "FOUND" << creatures_addrs.size() << "creatures";
    QTime t;
    t.start();
    if (!creatures_addrs.empty()) {
        Dwarf *d = 0;
        int progress_count = 0;

        foreach(VIRTADDR creature_addr, creatures_addrs) {
            d = Dwarf::get_dwarf(this, creature_addr);
            if(d){
                dwarves.append(d); //add animals as well so we can show them
                if(!d->is_animal()){
                    LOGI << "FOUND UNIT" << hexify(creature_addr) << d->nice_name() << d->id() << d->historical_id();
                    m_actual_dwarves.append(d);

                    //never calculate roles for babies
                    //only calculate roles for children if they're shown and labor cheats are enabled
                    if(!d->is_baby()){
                        if(!d->is_child() || (DT->labor_cheats_allowed() && !DT->hide_non_adults())){
                            //dwarves_with_roles.append(d->id());
                            m_labor_capable_dwarves.append(d);
                        }
                    }
                } else {
                    LOGI << "FOUND BEAST" << hexify(creature_addr) << d->nice_name() << d->id();
                }
            }
            emit progress_value(progress_count++);
        }
        LOGI << "read" << dwarves.count() << "units in" << t.elapsed() << "ms";

        m_enabled_labor_count.clear();
        qDeleteAll(m_pref_counts);
        m_pref_counts.clear();
        qDeleteAll(m_emotion_counts);
        m_emotion_counts.clear();
        m_equip_warning_counts.clear();

        t.restart();
        load_population_data();
        LOGI << "loaded population data in" << t.elapsed() << "ms";

        t.restart();
        load_role_ratings();
        LOGI << "calculated roles in" << t.elapsed() << "ms";

        //calc_done();
        m_actual_dwarves.clear();
        m_labor_capable_dwarves.clear();

        DT->emit_labor_counts_updated();

    } else {
        // we lost the fort!
        m_is_ok = false;
    }
    detach();

    LOGI << "found" << dwarves.size() << "dwarves out of" << creatures_addrs.size() << "creatures";

    return dwarves;
}

void DFInstance::load_population_data(){
    int labor_count = 0;
    int unit_kills = 0;
    int max_kills = 0;

    foreach(Dwarf *d, m_actual_dwarves){
        //load labor counts
        foreach(int key, d->get_labors().uniqueKeys()){
            if(d->labor_enabled(key)){
                if(m_enabled_labor_count.contains(key))
                    labor_count = m_enabled_labor_count.value(key)+1;
                else
                    labor_count = 1;
                m_enabled_labor_count.insert(key,labor_count);
            }
        }

        //save highest kill count
        unit_kills = d->hist_figure()->total_kills();
        if(unit_kills > max_kills)
            max_kills = unit_kills;

        //load preference/thoughts/item wear totals, excluding babies/children according to settings
        if(d->is_adult() || (!d->is_adult() && !DT->hide_non_adults())){
            foreach(QString category_name, d->get_grouped_preferences().uniqueKeys()){
                for(int i = 0; i < d->get_grouped_preferences().value(category_name)->count(); i++){

                    QString pref = d->get_grouped_preferences().value(category_name)->at(i);
                    QString cat_name = category_name;
                    bool is_dislike = false;
                    //put liked and hated creatures together
                    if(category_name == Preference::get_pref_desc(HATE_CREATURE)){
                        cat_name = Preference::get_pref_desc(LIKE_CREATURE);
                        is_dislike = true;
                    }

                    QPair<QString,QString> key_pair;
                    key_pair.first = cat_name;
                    key_pair.second = pref;

                    pref_stat *p;
                    if(m_pref_counts.contains(key_pair))
                        p = m_pref_counts.take(key_pair);
                    else{
                        p = new pref_stat();
                    }

                    if(is_dislike){
                        p->names_dislikes.append(d->nice_name());
                    }else{
                        p->names_likes.append(d->nice_name());
                    }
                    p->pref_category = cat_name;

                    m_pref_counts.insert(key_pair,p);
                }
            }

            //emotions
            QList<UnitEmotion*> d_emotions = d->get_emotions();
            foreach(UnitEmotion *ue, d_emotions){
                int thought_id = ue->get_thought_id();
                if(!m_emotion_counts.contains(thought_id)){
                    m_emotion_counts.insert(thought_id, new EmotionGroup(this));
                }
                EmotionGroup *em = m_emotion_counts.value(thought_id);
                em->add_detail(d,ue);
            }

            //inventory wear
            QPair<QString,int> wear_key;
            QHash<QPair<QString,int>,int> wear_counts = d->get_equip_warnings();
            foreach(wear_key, wear_counts.uniqueKeys()){
                int worn_count = wear_counts.value(wear_key);
                if(m_equip_warning_counts.contains(wear_key)){
                    m_equip_warning_counts[wear_key] += worn_count;
                }else{
                    m_equip_warning_counts.insert(wear_key,worn_count);
                }
            }
        }
    }
    DwarfStats::set_max_unit_kills(max_kills);
}

void DFInstance::load_role_ratings(){
    if(m_labor_capable_dwarves.size() <= 0)
        return;

    QVector<double> attribute_values;
    QVector<double> attribute_raw_values;
    QVector<double> skill_values;
    QVector<double> trait_values;
    QVector<double> pref_values;

    GameDataReader *gdr = GameDataReader::ptr();

    foreach(Dwarf *d, m_labor_capable_dwarves){
        foreach(ATTRIBUTES_TYPE id, gdr->get_attributes().keys()){
            attribute_values.append(d->get_attribute(id).get_balanced_value());
            attribute_raw_values.append(d->get_attribute(id).get_value());
        }

        foreach(int id, gdr->get_skills().keys()){
            skill_values.append(d->get_skill(id).get_balanced_level());
        }

        foreach(short val, d->get_traits()->values()){
            trait_values.append((double)val);
        }

        foreach(Role *r, gdr->get_roles().values()){
            if(r->prefs.count() > 0){
                pref_values.append(d->get_role_pref_match_counts(r,true));
            }
        }
    }

    QTime tr;
    tr.start();
    LOGD << "Role Trait Info:";
    DwarfStats::init_traits(trait_values);
    LOGD << "     - loaded trait role data in" << tr.elapsed() << "ms";

    LOGD << "Role Skills Info:";
    DwarfStats::init_skills(skill_values);
    LOGD << "     - loaded skill role data in" << tr.elapsed() << "ms";

    LOGD << "Role Attributes Info:";
    DwarfStats::init_attributes(attribute_values,attribute_raw_values);
    LOGD << "     - loaded attribute role data in" << tr.elapsed() << "ms";

    LOGD << "Role Preferences Info:";
    DwarfStats::init_prefs(pref_values);
    LOGD << "     - loaded preference role data in" << tr.elapsed() << "ms";

    float role_rating_avg = 0;
    bool calc_role_avg = (DT->get_log_manager()->get_appender("core")->minimum_level() <= LL_DEBUG);

    QVector<double> all_role_ratings;
    foreach(Dwarf *d, m_labor_capable_dwarves){
        foreach(double rating, d->calc_role_ratings()){
            all_role_ratings.append(rating);
            if(calc_role_avg)
                role_rating_avg+=rating;
        }
    }
    LOGD << "Role Display Info:";
    DwarfStats::init_roles(all_role_ratings);
    foreach(Dwarf *d, m_labor_capable_dwarves){
        d->refresh_role_display_ratings();
    }
    LOGD << "     - loaded role display data in" << tr.elapsed() << "ms";

    if(DT->get_log_manager()->get_appender("core")->minimum_level() <= LL_DEBUG){
        float max = 0;
        float min = 0;
        float median = 0;
        if(all_role_ratings.count() > 0){
            qSort(all_role_ratings);
            role_rating_avg /= all_role_ratings.count();
            max = all_role_ratings.last();
            min = all_role_ratings.first();
            median = RoleCalcBase::find_median(all_role_ratings);
        }
        LOGD << "Overall Role Rating Stats";
        LOGD << "     - Min: " << min;
        LOGD << "     - Max: " << max;
        LOGD << "     - Median: " << median;
        LOGD << "     - Average: " << role_rating_avg;
    }

}


void DFInstance::load_reactions(){
    attach();
    //LOGI << "Reading reactions names...";
    VIRTADDR reactions_vector = m_layout->address("reactions_vector");
    if(m_layout->is_valid_address(reactions_vector)){
        QVector<VIRTADDR> reactions = enumerate_vector(reactions_vector);
        //TRACE << "FOUND" << reactions.size() << "reactions";
        //emit progress_range(0, reactions.size()-1);
        if (!reactions.empty()) {
            foreach(VIRTADDR reaction_addr, reactions) {
                Reaction* r = Reaction::get_reaction(this, reaction_addr);
                m_reactions.insert(r->tag(), r);
                //emit progress_value(i++);
            }
        }
    }
    detach();
}

void DFInstance::load_main_vectors(){
    //material templates
    LOGD << "reading material templates";
    QVector<VIRTADDR> temps = enumerate_vector(m_layout->address("material_templates_vector"));
    foreach(VIRTADDR addr, temps){
        m_material_templates.insert(read_string(addr),addr);
    }

    //syndromes
    LOGD << "reading syndromes";
    m_all_syndromes = enumerate_vector(m_layout->address("all_syndromes_vector"));

    //load item types/subtypes
    LOGD << "reading item and subitem types";
    m_itemdef_vectors.insert(WEAPON,enumerate_vector(m_layout->address("itemdef_weapons_vector")));
    m_itemdef_vectors.insert(TRAPCOMP,enumerate_vector(m_layout->address("itemdef_trap_vector")));
    m_itemdef_vectors.insert(TOY,enumerate_vector(m_layout->address("itemdef_toy_vector")));
    m_itemdef_vectors.insert(TOOL,enumerate_vector(m_layout->address("itemdef_tool_vector")));
    m_itemdef_vectors.insert(INSTRUMENT,enumerate_vector(m_layout->address("itemdef_instrument_vector")));
    m_itemdef_vectors.insert(ARMOR,enumerate_vector(m_layout->address("itemdef_armor_vector")));
    m_itemdef_vectors.insert(AMMO,enumerate_vector(m_layout->address("itemdef_ammo_vector")));
    m_itemdef_vectors.insert(SIEGEAMMO,enumerate_vector(m_layout->address("itemdef_siegeammo_vector")));
    m_itemdef_vectors.insert(GLOVES,enumerate_vector(m_layout->address("itemdef_glove_vector")));
    m_itemdef_vectors.insert(SHOES,enumerate_vector(m_layout->address("itemdef_shoe_vector")));
    m_itemdef_vectors.insert(SHIELD,enumerate_vector(m_layout->address("itemdef_shield_vector")));
    m_itemdef_vectors.insert(HELM,enumerate_vector(m_layout->address("itemdef_helm_vector")));
    m_itemdef_vectors.insert(PANTS,enumerate_vector(m_layout->address("itemdef_pant_vector")));
    m_itemdef_vectors.insert(FOOD,enumerate_vector(m_layout->address("itemdef_food_vector")));

    LOGD << "reading colors, shapes, poems, music and dances";
    m_color_vector = enumerate_vector(m_layout->address("colors_vector"));
    m_shape_vector = enumerate_vector(m_layout->address("shapes_vector"));
    m_poetic_vector = enumerate_vector(m_layout->address("poetic_forms_vector"));
    m_music_vector = enumerate_vector(m_layout->address("musical_forms_vector"));
    m_dance_vector = enumerate_vector(m_layout->address("dance_forms_vector"));

    LOGD << "reading base materials";
    VIRTADDR addr = m_layout->address("base_materials");
    int i = 0;
    for(i = 0; i < 256; i++){
        VIRTADDR mat_addr = read_addr(addr);
        if(mat_addr > 0){
            Material* m = Material::get_material(this, mat_addr, i, false, this);
            m_base_materials.append(m);
        }
        addr += 0x4;
    }

    //inorganics
    LOGD << "reading inorganics";
    addr = m_layout->address("inorganics_vector");
    i = 0;
    foreach(VIRTADDR mat, enumerate_vector(addr)){
        //inorganic_raw.material
        Material* m = Material::get_material(this, mat, i, true, this);
        m_inorganics_vector.append(m);
        i++;
    }

    //plants
    LOGD << "reading plants";
    addr = m_layout->address("plants_vector");
    i = 0;
    QVector<VIRTADDR> vec = enumerate_vector(addr);
    foreach(VIRTADDR plant, vec){
        Plant* p = Plant::get_plant(this, plant, i);
        m_plants_vector.append(p);
        i++;
    }
}

ItemWeaponSubtype *DFInstance::find_weapon_subtype(QString name){
    foreach(ItemSubtype *i, m_item_subtypes.value(WEAPON)){
        ItemWeaponSubtype *w = qobject_cast<ItemWeaponSubtype*>(i);
        if(QString::compare(w->name_plural(),name,Qt::CaseInsensitive) == 0 ||
                QString::compare(w->group_name(),name,Qt::CaseInsensitive) == 0 ||
                w->group_name().contains(name,Qt::CaseInsensitive)){
            return w;
        }
    }
    return 0;
}

void DFInstance::load_item_defs(){
    foreach (const QList<ItemSubtype*> &list, m_item_subtypes) {
        foreach (ItemSubtype* def, list) {
            delete def;
        }
    }
    m_item_subtypes.clear();
    m_ordered_weapon_defs.clear();

    foreach(ITEM_TYPE itype, Item::items_with_subtypes()){
        QVector<VIRTADDR> addresses = m_itemdef_vectors.value(itype);
        if (!addresses.empty()) {
            foreach(VIRTADDR addr, addresses) {
                if(Item::is_armor_type(itype)){
                    m_item_subtypes[itype].append(new ItemArmorSubtype(itype,this,addr,this));
                }else if(itype == WEAPON){
                    ItemWeaponSubtype *w = new ItemWeaponSubtype(this,addr,this);
                    m_item_subtypes[itype].append(w);
                    m_ordered_weapon_defs.insert(w->name_plural(),w);
                }else if(itype == TOOL){
                    m_item_subtypes[itype].append(new ItemToolSubtype(this,addr,this));
                }else{
                    m_item_subtypes[itype].append(new ItemGenericSubtype(itype,this,addr,this));
                }
            }
        }
    }
}

ItemSubtype *DFInstance::get_item_subtype(ITEM_TYPE itype, int sub_type){
    if(m_item_subtypes.contains(itype)){
        QList<ItemSubtype*> list = m_item_subtypes.value(itype);
        if(list.size() > 0 && sub_type >= 0 && sub_type < list.size()){
            return list.at(sub_type);
        }
    }
    return 0;
}


void DFInstance::load_races_castes(){
    attach();
    VIRTADDR races_vector_addr = m_layout->address("races_vector");
    QVector<VIRTADDR> races = enumerate_vector(races_vector_addr);
    int idx = 0;
    if (!races.empty()) {
        foreach(VIRTADDR race_addr, races) {
            m_races.append(Race::get_race(this, race_addr, idx));
            idx++;
        }
    }
    detach();
}

const QString DFInstance::fortress_name(){
    QString name = m_fortress_name;
    if(!m_fortress_name_translated.isEmpty())
        name.append(QString(", \"%1\"").arg(m_fortress_name_translated));
    return name;
}

void DFInstance::refresh_data(){
    load_fortress();
    load_squads(true);
    load_items();
}

void DFInstance::load_items(){
    m_mapped_items.clear();
    m_items_vectors.clear();

    //these item vectors appear to contain unclaimed items!
    //load actual weapons and armor
    m_items_vectors.insert(WEAPON,enumerate_vector(m_layout->address("weapons_vector")));
    m_items_vectors.insert(SHIELD,enumerate_vector(m_layout->address("shields_vector")));
    m_items_vectors.insert(PANTS,enumerate_vector(m_layout->address("pants_vector")));
    m_items_vectors.insert(ARMOR,enumerate_vector(m_layout->address("armor_vector")));
    m_items_vectors.insert(SHOES,enumerate_vector(m_layout->address("shoes_vector")));
    m_items_vectors.insert(HELM,enumerate_vector(m_layout->address("helms_vector")));
    m_items_vectors.insert(GLOVES,enumerate_vector(m_layout->address("gloves_vector")));

    //load other equipment
    m_items_vectors.insert(QUIVER,enumerate_vector(m_layout->address("quivers_vector")));
    m_items_vectors.insert(BACKPACK,enumerate_vector(m_layout->address("backpacks_vector")));
    m_items_vectors.insert(CRUTCH,enumerate_vector(m_layout->address("crutches_vector")));
    m_items_vectors.insert(FLASK,enumerate_vector(m_layout->address("flasks_vector")));
    m_items_vectors.insert(AMMO,enumerate_vector(m_layout->address("ammo_vector")));

    //load artifacts
    m_items_vectors.insert(ARTIFACTS,enumerate_vector(m_layout->address("artifacts_vector")));
}

void DFInstance::load_fortress(){
    //load the fortress historical entity
    if(m_fortress){
        delete(m_fortress);
        m_fortress = 0;
    }
    VIRTADDR addr_fortress = m_layout->address("fortress_entity");
    m_fortress = FortressEntity::get_entity(this,read_addr(addr_fortress));
    if(m_fortress_name_translated.isEmpty())
        load_fortress_name();
}

void DFInstance::load_fortress_name(){
    //load the fortress name
    //fortress name is actually in the world data's site list
    //we can access a list of the currently active sites and read the name from there
    VIRTADDR world_data_addr = read_addr(m_layout->address("world_data"));
    QVector<VIRTADDR> sites = enumerate_vector(world_data_addr + m_layout->address("active_sites_vector",false));
    foreach(VIRTADDR site, sites){
        short t = read_short(site + m_layout->address("world_site_type",false));
        if(t==0){ //player fortress type
            m_fortress_name = get_language_word(site);
            m_fortress_name_translated = get_translated_word(site);
            break;
        }
    }
}

QList<Squad *> DFInstance::load_squads(bool show_progress) {
    QList<Squad*> squads;
    if (!m_is_ok) {
        LOGW << "not connected";
        detach();
        return squads;
    }

    if(show_progress){
        // we're connected, make sure we have good addresses
        m_squad_vector = m_layout->address("squad_vector");
        if(m_squad_vector == 0xFFFFFFFF) {
            LOGI << "Squads not supported for this version of Dwarf Fortress";
            return squads;
        }
        LOGD << "loading squads from " << hexify(m_squad_vector);
        emit progress_message(tr("Loading Squads"));
    }

    attach();

    QVector<VIRTADDR> squads_addr = enumerate_vector(m_squad_vector);
    LOGI << "FOUND" << squads_addr.size() << "squads";

    qDeleteAll(m_squads);
    m_squads.clear();

    if (!squads_addr.empty()) {
        if(show_progress)
            emit progress_range(0, squads_addr.size()-1);

        int squad_count = 0;
        foreach(VIRTADDR squad_addr, squads_addr) {
            int id = read_int(squad_addr + m_layout->squad_offset("id")); //check the id before loading the squad
            if(m_fortress->squad_is_active(id)){
                Squad *s = new Squad(id, this, squad_addr);
                LOGI << "FOUND ACTIVE SQUAD" << hexify(squad_addr) << s->name() << " member count: " << s->assigned_count() << " id: " << s->id();
                if (m_fortress->squad_is_active(s->id())) {
                    m_squads.push_front(s);
                } else {
                    delete s;
                }
            }

            if(show_progress)
                emit progress_value(squad_count++);
        }
    }

    detach();
    //LOGI << "Found" << squads.size() << "squads out of" << m_current_creatures.size();
    return m_squads;
}

Squad* DFInstance::get_squad(int id){
    foreach(Squad *s, m_squads){
        if(s->id() == id)
            return s;
    }
    return 0;
}


void DFInstance::heartbeat() {
    // simple read attempt that will fail if the DF game isn't running a fort, or isn't running at all
    // it would be nice to find a less cumbersome read, but for now at least we know this works
    if(get_creatures(false).size() < 1){
        // no game loaded, or process is gone
        emit connection_interrupted();
    }
}

QVector<VIRTADDR> DFInstance::get_creatures(bool report_progress){
    VIRTADDR active_units = m_layout->address("active_creature_vector");
    VIRTADDR all_units = m_layout->address("creature_vector");

    //first try the active unit list
    QVector<VIRTADDR> entries = enumerate_vector(active_units);
    if(entries.isEmpty()){
        if(report_progress){
            LOGI << "no active units (embark) using full unit list";
        }
        entries = enumerate_vector(all_units);
    }else{
        //there are active units, but are they ours?
        int civ_offset = m_layout->dwarf_offset("civ");
        foreach(VIRTADDR entry, entries){
            if(read_word(entry + civ_offset)==m_dwarf_civ_id){
                if(report_progress){
                    LOGI << "using active units";
                }
                return entries;
            }
        }
        if(report_progress){
            LOGI << "no active units with our civ (reclaim), using full unit list";
        }
        entries = enumerate_vector(all_units);
    }
    return entries;
}

QString DFInstance::pprint(const QByteArray &ba) {
    QString out = "    ADDR   | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F | TEXT\n";
    out.append("------------------------------------------------------------------------\n");
    int lines = ba.size() / 16;
    if (ba.size() % 16)
        lines++;
    if (lines < 1)
        lines = 0;

    for(int i = 0; i < lines; ++i) {
        VIRTADDR offset = i * 16;
        out.append(hexify(offset));
        out.append(" | ");
        for (int c = 0; c < 16; ++c) {
            out.append(ba.mid(i*16 + c, 1).toHex());
            out.append(" ");
        }
        out.append("| ");
        for (int c = 0; c < 16; ++c) {
            QByteArray tmp = ba.mid(i*16 + c, 1);
            if (tmp.at(0) == 0)
                out.append(".");
            else if (tmp.at(0) <= 126 && tmp.at(0) >= 32)
                out.append(tmp);
            else
                out.append(tmp.toHex());
        }
        //out.append(ba.mid(i*16, 16).toPercentEncoding());
        out.append("\n");
    }
    return out;
}

Word * DFInstance::read_dwarf_word(const VIRTADDR &addr) {
    Word * result = NULL;
    uint word_id = read_int(addr);
    if(word_id != 0xFFFFFFFF) {
        result = DT->get_word(word_id);
    }
    return result;
}

QString DFInstance::read_dwarf_name(const VIRTADDR &addr) {
    QString result = "The";

    //7 parts e.g.  ffffffff ffffffff 000006d4
    //      ffffffff ffffffff 000002b1 ffffffff

    //Unknown
    Word * word = read_dwarf_word(addr);
    if(word)
        result.append(" " + capitalize(word->base()));

    //Unknown
    word = read_dwarf_word(addr + 0x04);
    if(word)
        result.append(" " + capitalize(word->base()));

    //Verb
    word = read_dwarf_word(addr + 0x08);
    if(word) {
        result.append(" " + capitalize(word->adjective()));
    }

    //Unknown
    word = read_dwarf_word(addr + 0x0C);
    if(word)
        result.append(" " + capitalize(word->base()));

    //Unknown
    word = read_dwarf_word(addr + 0x10);
    if(word)
        result.append(" " + capitalize(word->base()));

    //Noun
    word = read_dwarf_word(addr + 0x14);
    bool singular = false;
    if(word) {
        if(word->plural_noun().isEmpty()) {
            result.append(" " + capitalize(word->noun()));
            singular = true;
        } else {
            result.append(" " + capitalize(word->plural_noun()));
        }
    }

    //of verb(noun)
    word = read_dwarf_word(addr + 0x18);
    if(word) {
        if( !word->verb().isEmpty() ) {
            if(singular) {
                result.append(" of " + capitalize(word->verb()));
            } else {
                result.append(" of " + capitalize(word->present_participle_verb()));
            }
        } else {
            if(singular) {
                result.append(" of " + capitalize(word->noun()));
            } else {
                result.append(" of " + capitalize(word->plural_noun()));
            }
        }
    }

    return result.trimmed();
}


MemoryLayout *DFInstance::get_memory_layout(QString checksum, bool) {
    checksum = checksum.toLower();
    LOGI << "DF's checksum is:" << checksum;

    MemoryLayout *ret_val = NULL;
    ret_val = m_memory_layouts.value(checksum, NULL);
    m_is_ok = ret_val != NULL && ret_val->is_valid();

    if(!m_is_ok) {
        LOGI << "Could not find layout for checksum" << checksum;
        DT->get_main_window()->check_for_layout(checksum);
    }

    if (m_is_ok) {
        LOGI << "Detected Dwarf Fortress version"
             << ret_val->game_version() << "using MemoryLayout from"
             << ret_val->filename();
    }

    return ret_val;
}

bool DFInstance::add_new_layout(const QString & version, QFile & file) {
    QString newFileName = version;
    newFileName.replace("(", "").replace(")", "").replace(" ", "_");
    newFileName +=  ".ini";

    QFileInfo newFile(QDir(QString("share/memory_layouts/%1").arg(LAYOUT_SUBDIR)), newFileName);
    newFileName = newFile.absoluteFilePath();

    if(!file.exists()) {
        LOGW << "Layout file" << file.fileName() << "does not exist!";
        return false;
    }

    LOGI << "Copying: " << file.fileName() << " to " << newFileName;
    if(!file.copy(newFileName)) {
        LOGW << "Error renaming layout file!";
        return false;
    }

    MemoryLayout *temp = new MemoryLayout(newFileName);
    if(temp && temp->is_valid()) {
        LOGI << "adding valid layout" << temp->game_version() << temp->checksum();
        m_memory_layouts.insert(temp->checksum().toLower(), temp);
    }else{
        LOGI << "ignoring invalid layout from file:" << newFileName;
        delete temp;
    }
    return true;
}

void DFInstance::layout_not_found(const QString & checksum) {
    QString supported_vers;

    // TODO: Replace this with a rich dialog at some point that
    // is also accessible from the help menu. For now, remove the
    // extra path information as the dialog is getting way too big.
    // And make a half-ass attempt at sorting
    QList<MemoryLayout *> layouts = m_memory_layouts.values();
    qSort(layouts);

    foreach(MemoryLayout * l, layouts) {
        supported_vers.append(
                    QString("<li><b>%1</b>(<font color=\"#444444\">%2"
                            "</font>)</li>")
                    .arg(l->game_version())
                    .arg(l->checksum()));
    }

    QMessageBox *mb = new QMessageBox(qApp->activeWindow());
    mb->setIcon(QMessageBox::Critical);
    mb->setWindowTitle(tr("Unidentified Game Version"));
    mb->setText(tr("I'm sorry but I don't know how to talk to this "
                   "version of Dwarf Fortress! (checksum:%1)<br><br> <b>Supported "
                   "Versions:</b><ul>%2</ul>").arg(checksum).arg(supported_vers));

    /*
    mb->setDetailedText(tr("Failed to locate a memory layout file for "
        "Dwarf Fortress exectutable with checksum '%1'").arg(checksum));
    */
    mb->exec();
    LOGE << tr("unable to identify version from checksum:") << checksum;
}


VIRTADDR DFInstance::find_historical_figure(int hist_id){
    if(m_hist_figures.count() <= 0)
        load_hist_figures();

    return m_hist_figures.value(hist_id,0);
}

void DFInstance::load_hist_figures(){
    QVector<VIRTADDR> hist_figs = enumerate_vector(m_layout->address("historical_figures_vector"));
    foreach(VIRTADDR fig, hist_figs){
        m_hist_figures.insert(read_int(fig + m_layout->hist_figure_offset("id")),fig);
    }
}

VIRTADDR DFInstance::find_identity(int id){
    if(m_fake_identities.count() == 0) //lazy load fake identities
        m_fake_identities = enumerate_vector(m_layout->address("fake_identities_vector"));
    foreach(VIRTADDR ident, m_fake_identities){
        int fake_id = read_int(ident);
        if(fake_id==id){
            return ident;
        }
    }
    return 0;
}

VIRTADDR DFInstance::find_event(int id){
    if(m_events.count() == 0){
        QVector<VIRTADDR> all_events_addrs = enumerate_vector(m_layout->address("events_vector"));
        foreach(VIRTADDR evt_addr, all_events_addrs){
            m_events.insert(read_int(evt_addr+m_layout->hist_event_offset("id")),evt_addr);
        }
    }
    return m_events.value(id,0);
}

QVector<VIRTADDR> DFInstance::get_item_vector(ITEM_TYPE i){
    if(m_itemdef_vectors.contains(i))
        return m_itemdef_vectors.value(i);
    else
        return m_itemdef_vectors.value(NONE);
}

QString DFInstance::get_preference_item_name(int index, int subtype){
    ITEM_TYPE itype = static_cast<ITEM_TYPE>(index);

    if(Item::has_subtypes(itype)){
        QList<ItemSubtype*> list = m_item_subtypes.value(itype);
        if(!list.isEmpty() && (subtype >=0 && subtype < list.count()))
            return list.at(subtype)->name_plural();
    }else{
        QVector<VIRTADDR> addrs = get_item_vector(itype);
        if(!addrs.empty() && (subtype >=0 && subtype < addrs.count()))
            return read_string(addrs.at(subtype) + m_layout->item_subtype_offset("name_plural"));
    }

    return Item::get_item_name_plural(itype);
}

VIRTADDR DFInstance::get_item_address(ITEM_TYPE itype, int item_id){
    if(m_mapped_items.value(itype).count() <= 0)
        index_item_vector(itype);
    if(m_mapped_items.contains(itype)){
        if(m_mapped_items.value(itype).contains(item_id))
            return m_mapped_items.value(itype).value(item_id);
    }
    return 0;
}

QString DFInstance::get_artifact_name(ITEM_TYPE itype, int item_id){
    if(m_mapped_items.value(itype).count() <= 0)
        index_item_vector(itype);

    if(itype == ARTIFACTS){
        VIRTADDR addr = m_mapped_items.value(itype).value(item_id);
        QString name = read_dwarf_name(addr+0x4);
        name = get_language_word(addr+0x4);
        return name;
    }else{
        return "";
    }
}

void DFInstance::index_item_vector(ITEM_TYPE itype){
    QHash<int,VIRTADDR> items;
    int offset = m_layout->item_offset("id");
    if(itype == ARTIFACTS){
        offset = 0x0;
    }
    foreach(VIRTADDR addr, m_items_vectors.value(itype)){
        items.insert(read_int(addr+offset),addr);
    }
    m_mapped_items.insert(itype,items);
}

QString DFInstance::get_preference_other_name(int index, PREF_TYPES p_type){
    QVector<VIRTADDR> target_vec;
    int offset = 0x4; //default for poem/music/dance
    bool translate = true;

    if(p_type == LIKE_SHAPE || p_type == LIKE_COLOR){
        translate = false;
        if(LIKE_COLOR){
            target_vec = m_color_vector;
            offset = m_layout->descriptor_offset("color_name");
        }else{
            target_vec = m_shape_vector;
            offset = m_layout->descriptor_offset("shape_name_plural");
        }
    }

    if(p_type == LIKE_POETRY || p_type == LIKE_MUSIC || p_type == LIKE_DANCE){
        if(p_type == LIKE_POETRY)
            target_vec = m_poetic_vector;
        if(p_type == LIKE_MUSIC)
            target_vec = m_music_vector;
        if(p_type == LIKE_DANCE)
            target_vec = m_dance_vector;
    }

    if(index > -1 && index < target_vec.count()){
        VIRTADDR addr = target_vec.at(index);
        if(translate){
            return get_translated_word(addr + offset);
        }else{
            return read_string(addr + offset);
        }
    }else{
        return "unknown";
    }
}

QString DFInstance::find_material_name(int mat_index, short mat_type, ITEM_TYPE itype, MATERIAL_STATES mat_state){
    Material *m = find_material(mat_index, mat_type);
    QString name = "";

    if (!m)
        return name;

    if (mat_index < 0 || mat_type < 19) {
        name = m->get_material_name(mat_state);
    }
    else if(mat_type < 219){
        if(itype == DRINK || itype == LIQUID_MISC){
            name = m->get_material_name(LIQUID);
        }else if(itype == CHEESE)
            name = m->get_material_name(mat_state);
        else
        {
            name = "";
            Race* r = get_race(mat_index);
            if(r){
                name = r->name().toLower().append(" ");
            }
            name.append(m->get_material_name(mat_state));
        }
    }
    else if(mat_type < 419)
    {
        VIRTADDR hist_figure = find_historical_figure(mat_index);
        if(hist_figure){
            Race *r = get_race(read_short(hist_figure + m_layout->hist_figure_offset("hist_race")));
            if(r){
                name = QString(tr("%1's %2"))
                        .arg(read_string(hist_figure + m_layout->hist_figure_offset("hist_name")))
                        .arg(m->get_material_name(mat_state));
            }
        }
    }
    else if(mat_type < 619){
        Plant *p = get_plant(mat_index);
        if(p){
            name = p->name();

            if(itype==LEAVES_FRUIT)
                name = p->leaf_plural();
            else if(itype==SEEDS)
                name = p->seed_plural();
            else if(itype==PLANT)
                name = p->name_plural();

            //specific plant material
            if(m){
                if(itype == DRINK || itype == LIQUID_MISC){
                    name = m->get_material_name(LIQUID);
                }else if(itype == POWDER_MISC || itype == CHEESE){
                    name = m->get_material_name(POWDER);
                }else if(Item::is_armor_type(itype)){
                    //don't include the 'fabric' part if it's a armor (item?) ie. pig tail fiber coat, not pig tail fiber fabric coat
                    //this appears to have changed now (42.x) and the solid name is used simply: pig tail coat
                    name = m->get_material_name(SOLID);
                }else if(mat_state != SOLID){
                    name = m->get_material_name(mat_state);
                }else if(m->flags().has_flag(LEAF_MAT) && m->flags().has_flag(STRUCTURAL_PLANT_MAT)){
                    name = p->name().append(" ").append(m->get_material_name(GENERIC));//fruit
                }else if(itype == NONE || m->flags().has_flag(IS_WOOD)){
                    if(p->flags().has_flag(P_TREE)){
                        name.append(" ").append(m->get_material_name(GENERIC));
                    }else{
                        name = m->get_material_name(SOLID) + " " + m->get_material_name(GENERIC);
                    }
                }

            }
        }
    }
    return name.toLower().trimmed();
}

Material *DFInstance::find_material(int mat_index, short mat_type){
    if (mat_index < 0) {
        return get_raw_material(mat_type);
    } else if (mat_type == 0) {
        return get_inorganic_material(mat_index);
    } else if (mat_type < 19) {
        return get_raw_material(mat_type);
    } else if (mat_type < 219) {
        Race* r = get_race(mat_index);
        if (r)
            return r->get_creature_material(mat_type - 19);
    } else if (mat_type < 419) {
        VIRTADDR hist_figure = find_historical_figure(mat_index);
        if (hist_figure) {
            Race *r = get_race(read_short(hist_figure + m_layout->hist_figure_offset("hist_race")));
            if (r)
                return r->get_creature_material(mat_type-219);
        }
    } else if (mat_type < 619) {
        Plant *p = get_plant(mat_index);
        int index = mat_type - 419;
        if (p && index < p->material_count())
            return p->get_plant_material(index);
    }
    return NULL;
}

bool DFInstance::authorize() {
    return true;
}
