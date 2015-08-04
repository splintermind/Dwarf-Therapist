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
#include "truncatingfilelogger.h"
#include "dwarfjob.h"

DwarfJob::DwarfJob(short id, QString description, DWARF_JOB_TYPE type, QString reactionClass, QObject *parent)
    : QObject(parent)
    , id(id)
    , description(description)
    , type(type)
    , reactionClass(reactionClass)
{}

const QHash<QString, DwarfJob::DWARF_JOB_TYPE> DwarfJob::jobTypes{
    {"idle", DJT_IDLE},
    {"dig", DJT_DIG},
    {"cut", DJT_CUT},
    {"sleep", DJT_SLEEP},
    {"drink", DJT_DRINK},
    {"food", DJT_FOOD},
    {"build", DJT_BUILD},
    {"haul", DJT_HAUL},
    {"admin", DJT_ADMIN},
    {"fight", DJT_FIGHT},
    {"mood", DJT_MOOD},
    {"forge", DJT_FORGE},
    {"medical", DJT_MEDICAL},
    {"furnace", DJT_FURNACE},
    {"wax_working", DJT_WAX_WORKING},
    {"bee_keeping", DJT_BEE_KEEPING},
    {"pressing", DJT_PRESSING},
    {"spinning", DJT_SPINNING},
    {"pottery", DJT_POTTERY},
    {"glazing", DJT_POTTERY},
    {"stair", DJT_STAIRS},
    {"fortification", DJT_FORTIFICATION},
    {"engrave", DJT_ENGRAVE},
    {"leaf", DJT_LEAF},
    {"build_remove", DJT_BUILD_REMOVE},
    {"bag_add", DJT_BAG_ADD},
    {"money", DJT_MONEY},
    {"tax", DJT_TAX},
    {"return", DJT_RETURN},
    {"party", DJT_PARTY},
    {"soap", DJT_SOAP},
    {"seek", DJT_SEEK},
    {"gem_cut", DJT_GEM_CUT},
    {"gem_encrust", DJT_GEM_ENCRUST},
    {"seeds", DJT_SEEDS},
    {"harvest", DJT_LEAF_ARROW},
    {"give_water", DJT_WATER_ARROW},
    {"tombstone", DJT_TOMBSTONE},
    {"animal", DJT_ANIMAL},
    {"book", DJT_BOOK_OPEN},
    {"construct", DJT_CONSTRUCT},
    {"handshake", DJT_HANDSHAKE},
    {"abacus", DJT_ABACUS},
    {"report", DJT_REPORT},
    {"justice", DJT_JUSTICE},
    {"shield", DJT_SHIELD},
    {"depot", DJT_DEPOT},
    {"broom", DJT_BROOM},
    {"switch", DJT_SWITCH},
    {"chain", DJT_CHAIN},
    {"unchain", DJT_UNCHAIN},
    {"fill_water", DJT_FILL_WATER},
    {"market", DJT_MARKET},
    {"knife", DJT_KNIFE},
    {"bow", DJT_BOW},
    {"milk", DJT_MILK},
    {"cheese", DJT_CHEESE},
    {"glove", DJT_GLOVE},
    {"boot", DJT_BOOT},
    {"armor", DJT_ARMOR},
    {"helm", DJT_HELM},
    {"fish", DJT_FISH},
    {"rawfish", DJT_RAW_FISH},
    {"rest", DJT_REST},
    {"cooking", DJT_COOKING},
    {"bucket_pour", DJT_BUCKET_POUR},
    {"give_love", DJT_GIVE_LOVE},
    {"weapon", DJT_WEAPON},
    {"dye", DJT_DYE},
    {"switch_connect", DJT_SWITCH_CONNECT},
    {"zone_add", DJT_ZONE_ADD},
    {"crafts", DJT_CRAFTS},
    {"gear", DJT_GEAR},
    {"trouble", DJT_TROUBLE},
    {"storage", DJT_STORAGE},
    {"store_owned", DJT_STORE_OWNED},
    {"brew", DJT_BREW},
    {"cabinet_store", DJT_CABINET_STORE},
    {"cabinet_make", DJT_CABINET_MAKE},
    {"door_make", DJT_DOOR_MAKE},
    {"chair_make", DJT_CHAIR_MAKE},
    {"soldier", DJT_SOLDIER},
    {"on_break", DJT_ON_BREAK},
    {"caged", DJT_CAGED}
};

DwarfJob::DWARF_JOB_TYPE DwarfJob::get_type(const QString &type) {
    return jobTypes.value(type.toLower(), DJT_DEFAULT);
}

const QString DwarfJob::get_job_mat_category_name(quint32 flags){
    switch(flags){
    case 0:
        return tr("plant");
    case 2:
        return tr("wood");
    case 4:
        return tr("cloth");
    case 8:
        return tr("silk");
    case 16:
        return tr("leather");
    case 32:
        return tr("bone");
    case 64:
        return tr("shell");
    case 128:
        return tr("wood mat");
    case 256:
        return tr("soap");
    case 512:
        return tr("ivory/tooth");
    case 1024:
        return tr("horn/hoof");
    case 2048:
        return tr("pearl");
    case 4096:
        return tr("yarn/wool/fur");
    }

    return "unknown";
}
