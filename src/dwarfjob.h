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
#ifndef DWARF_JOB_H
#define DWARF_JOB_H

#include "truncatingfilelogger.h"
#include <QString>
#include <QObject>

class DwarfJob : public QObject {
    Q_OBJECT
public:
    typedef enum {
        DJT_DEFAULT,
        DJT_IDLE,
        DJT_DIG,
        DJT_CUT,
        DJT_REST,
        DJT_DRINK,
        DJT_FOOD,
        DJT_BUILD,
        DJT_HAUL,
        DJT_ADMIN,
        DJT_FIGHT,
        DJT_MOOD,
        DJT_FORGE,
        DJT_MEDICAL,
        DJT_FURNACE,
        DJT_WAX_WORKING,
        DJT_BEE_KEEPING,
        DJT_PRESSING,
        DJT_SPINNING,
        DJT_POTTERY,
        DJT_STAIRS,
        DJT_FORTIFICATION,
        DJT_ENGRAVE,
        DJT_LEAF,
        DJT_BUILD_REMOVE,
        DJT_BAG_ADD,
        DJT_MONEY,
        DJT_RETURN,
        DJT_PARTY,
        DJT_SOAP,
        DJT_SEEK,
        DJT_GEM_CUT,
        DJT_GEM_ENCRUST,
        DJT_SEEDS,
        DJT_LEAF_ARROW,
        DJT_WATER_ARROW,
        DJT_TOMBSTONE,
        DJT_ANIMAL,
        DJT_BOOK_OPEN,
        DJT_HANDSHAKE,
        DJT_CONSTRUCT,
        DJT_ABACUS,
        DJT_REPORT,
        DJT_JUSTICE,
        DJT_SHIELD,
        DJT_DEPOT,
        DJT_BROOM,
        DJT_SWITCH,
        DJT_CHAIN,
        DJT_UNCHAIN,
        DJT_FILL_WATER,
        DJT_MARKET,
        DJT_KNIFE,
        DJT_BOW,
        DJT_MILK,
        DJT_CHEESE,
        DJT_GLOVE,
        DJT_BOOT,
        DJT_ARMOR,
        DJT_HELM,
        DJT_FISH,
        DJT_SLEEP,
        DJT_COOKING,
        DJT_BUCKET_POUR,
        DJT_GIVE_LOVE,
        DJT_DYE,
        DJT_WEAPON,
        DJT_SWITCH_CONNECT,
        DJT_ZONE_ADD,
        DJT_CRAFTS,
        DJT_GEAR,
        DJT_TROUBLE,
        DJT_STORAGE,
        DJT_STORE_OWNED,
        DJT_BREW,
        DJT_RAW_FISH,
        DJT_TAX,
        DJT_CABINET_STORE,
        DJT_CABINET_MAKE,
        DJT_DOOR_MAKE,
        DJT_CHAIR_MAKE,
        DJT_SOLDIER,
        DJT_ON_BREAK,
        DJT_CAGED
    } DWARF_JOB_TYPE;

    DwarfJob(short id, QString description, DWARF_JOB_TYPE type, QString reactionClass, QObject *parent = 0);

    static const QHash<QString, DWARF_JOB_TYPE> jobTypes;

    static DWARF_JOB_TYPE get_type(const QString &type);
    static const QString get_job_mat_category_name(quint32 flags);

    short id;
    QString description;
    DWARF_JOB_TYPE type;
    QString reactionClass;

};

#endif
