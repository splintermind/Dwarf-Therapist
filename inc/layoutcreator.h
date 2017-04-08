#ifndef LAYOUT_CREATOR_H
#define LAYOUT_CREATOR_H

#include <QObject>
#include "dfinstance.h"

class MemoryLayout;

class LayoutCreator : public QObject {
    Q_OBJECT
public:
    LayoutCreator(DFInstance *m_df, MemoryLayout * parent, QString file_name, QString version_name);
    virtual ~LayoutCreator();

    bool write_file();

protected:
    DFInstance *m_df;
    MemoryLayout * m_parent;
    QString m_file_name;
    QString m_version_name;

    VIRTADDR m_dwarf_race_index;
    VIRTADDR m_translation_vector;
    VIRTADDR m_language_vector;
    VIRTADDR m_creature_vector;
    VIRTADDR m_squad_vector;
    VIRTADDR m_current_year;

private slots:
    void report_global_address(const QString&, const VIRTADDR&);
};

#endif // LAYOUT_CREATOR_H
