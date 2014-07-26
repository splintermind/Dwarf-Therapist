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
#include <QtWidgets>

#include "dwarftherapist.h"
#include "mainwindow.h"
#include "optionsmenu.h"
#include "version.h"
#include "customprofession.h"
#include "superlabor.h"
#include "dwarfmodel.h"
#include "dwarfmodelproxy.h"
#include "dwarf.h"
#include "word.h"
#include "ui_mainwindow.h"
#include "dfinstance.h"
#include "memorylayout.h"
#include "truncatingfilelogger.h"
#include "viewmanager.h"
#include "dwarfstats.h"
#include "defaultfonts.h"


DwarfTherapist::DwarfTherapist(int &argc, char **argv)
    : QApplication(argc, argv)
    , multiple_castes(false)
    , show_skill_learn_rates(false)
    , arena_mode(false) //manually set this to true to do arena testing (very hackish, all units will be animals)
    , m_user_settings(0)
    , m_main_window(0)
    , m_options_menu(0)
    , m_reading_settings(false)
    , m_allow_labor_cheats(false)
    , m_hide_non_adults(false)
    , m_log_mgr(0)
{
    setup_logging();
    load_translator();

    TRACE << "Creating settings object";
    m_user_settings = new QSettings(QSettings::IniFormat, QSettings::UserScope, COMPANY, PRODUCT, this);
//    QString local_path = QApplication::applicationDirPath() + "/Dwarf Therapist.ini";
//    if(!QFile::exists(local_path)){
//        LOGI << "Local settings file not found, attempting to copy from user directory...";
//        m_user_settings = new QSettings(QSettings::IniFormat, QSettings::UserScope, COMPANY, PRODUCT, this);
//        if(QFile::copy(m_user_settings->fileName(), local_path)){
//            m_user_settings = new QSettings(local_path, QSettings::IniFormat, this);
//        }
//    }else{
//        //backup
//        LOGI << "Using and backing up local Dwarf Therapist.ini";
//        QFile::copy(local_path, QApplication::applicationDirPath() + "/Dwarf Therapist.ini.bak");
//        m_user_settings = new QSettings(local_path, QSettings::IniFormat, this);
//    }

//    if(!m_user_settings){
//        LOGI << "Failed to copy and/or open local Dwarf Therapist.ini, falling back to user ini.";
//        m_user_settings = new QSettings(QSettings::IniFormat, QSettings::UserScope, COMPANY, PRODUCT, this);
//    }



    TRACE << "Creating options menu";
    m_options_menu = new OptionsMenu;   

    TRACE << "Creating main window";
    m_main_window = new MainWindow;
    m_options_menu->setParent(m_main_window, Qt::Dialog);

    TRACE << "connecting signals";
    connect(m_options_menu, SIGNAL(settings_changed()), SIGNAL(settings_changed())); // the telephone game...
    connect(m_options_menu, SIGNAL(settings_changed()), this, SLOT(read_settings()));
    connect(m_main_window->ui->act_options, SIGNAL(triggered()), m_options_menu, SLOT(exec()));
    connect(m_main_window->ui->act_import_existing_professions, SIGNAL(triggered()), this, SLOT(import_existing_professions()));
    connect(m_main_window->ui->tree_custom_professions, SIGNAL(itemActivated(QTreeWidgetItem*,int)), this, SLOT(edit_customization(QTreeWidgetItem*)));
    connect(m_main_window->ui->act_add_custom_profession, SIGNAL(triggered()), this, SLOT(add_custom_profession()));
    connect(m_main_window->ui->le_filter_text, SIGNAL(textChanged(const QString&)), m_main_window->get_proxy(), SLOT(setFilterFixedString(const QString&)));

    read_settings();

    bool read = m_user_settings->value("options/read_on_startup", true).toBool();
    if (read) {
        QTimer::singleShot(0, m_main_window, SLOT(connect_to_df()));
    }
    m_main_window->show();
}

DwarfTherapist::~DwarfTherapist(){
    qDeleteAll(m_language);
    qDeleteAll(m_custom_prof_icns);
    qDeleteAll(m_custom_professions);

    delete m_user_settings;
    delete m_options_menu;
    delete m_main_window;
    delete m_log_mgr;
}

void DwarfTherapist::setup_logging() {
    QStringList args = arguments();
    bool debug_logging = args.indexOf("-debug") != -1;
    bool trace_logging = args.indexOf("-trace") != -1;

    LOG_LEVEL min_level = LL_INFO;
    if (trace_logging) {
        min_level = LL_TRACE;
    } else if (debug_logging) {
        min_level = LL_DEBUG;
    }

    //setup logging
    m_log_mgr = new LogManager(this);
    TruncatingFileLogger *log = m_log_mgr->add_logger("log/run.log");
    if (log) {
        LogAppender *app = m_log_mgr->add_appender("core", log, LL_TRACE);
        if (app) {
            Version v; // current version
            LOGI << "Dwarf Therapist" << v.to_string() << "starting normally.";
            //app->set_minimum_level(min_level);
            app->set_minimum_level(min_level);
        } else {
            qCritical() << "Could not open logfile!";
            qApp->exit(1);
        }
    } else {
        qCritical() << "Could not open logfile!";
        qApp->exit(1);
    }
}

QList<Dwarf*> DwarfTherapist::get_dwarves(){
    return m_main_window->get_model()->get_dwarves();
}

void DwarfTherapist::load_translator() {
    TRACE << "loading translations";
    QTranslator translator;
    translator.load("dwarftherapist_en");
    installTranslator(&translator);
    TRACE << "english translation loaded";
}

void DwarfTherapist::read_settings() {
    LOGI << "beginning to read settings";
    m_reading_settings = true; // don't allow writes while we're reading...

    // HACK!
    if (m_user_settings->value("it_feels_like_the_first_time", true).toBool() ||
            !m_user_settings->contains("options/colors/happiness/1") ||
            !m_user_settings->contains("options/colors/nobles/1")) {
        m_options_menu->write_settings(); //write it out so that we can get default colors loaded
        emit settings_changed(); // this will cause delegates to get the right default colors
        m_user_settings->setValue("it_feels_like_the_first_time", false);
    }    

    if (m_user_settings->value("options/show_toolbutton_text", true).toBool()) {
        m_main_window->get_toolbar()->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    } else {
        m_main_window->get_toolbar()->setToolButtonStyle(Qt::ToolButtonIconOnly);
    }

    qDeleteAll(m_custom_professions);
    m_custom_professions.clear();    
    qDeleteAll(m_custom_prof_icns);
    m_custom_prof_icns.clear();
    qDeleteAll(m_super_labors);
    m_super_labors.clear();

    m_user_settings->beginGroup("custom_professions");
    {
        QStringList profession_names = m_user_settings->childGroups();
        foreach(QString prof, profession_names) {
            CustomProfession *cp = new CustomProfession(prof, *m_user_settings, this);
            if(cp->prof_id() > -1){
                m_custom_prof_icns.insert(cp->prof_id(), cp);//profession icon override
            }else{
                m_custom_professions.insert(prof,cp);//custom profession
            }
            //add an automatic superlabor for the custom profession
            m_super_labors.insert(cp->get_name(), new SuperLabor(cp,this));
        }
    }
    m_user_settings->endGroup();

    //read in the custom superlabors
    int size = m_user_settings->beginReadArray("super_labors");
    {
        for(int idx = 0; idx < size; idx++) {
            m_user_settings->setArrayIndex(idx);
            SuperLabor *sl = new SuperLabor(*m_user_settings,this);
            m_super_labors.insert(sl->get_name(),sl);            
        }
    }
    m_user_settings->endArray();

    m_allow_labor_cheats = m_user_settings->value("options/allow_labor_cheats", false).toBool();
    m_hide_non_adults = m_user_settings->value("options/hide_children_and_babies",false).toBool();

    m_reading_settings = false;
    m_main_window->load_customizations();

    QApplication::setFont(DT->user_settings()->value("options/main_font", QFont(DefaultFonts::getMainFontName(), DefaultFonts::getMainFontSize())).value<QFont>());    
    //set the application's tooltips
    QToolTip::setFont(DT->user_settings()->value("options/tooltip_font", QFont(DefaultFonts::getTooltipFontName(), DefaultFonts::getTooltipFontSize())).value<QFont>());    

    //set a variable we'll use in the dwarfstats for role calcs
    DwarfStats::set_att_potential_weight(DT->user_settings()->value("options/default_attribute_potential_weight",0.5f).toFloat());
    DwarfStats::set_skill_rate_weight(DT->user_settings()->value("options/default_skill_rate_weight",0.25f).toFloat());
    LOGI << "finished reading settings";
}

void DwarfTherapist::emit_units_refreshed(){
    //perform anything that requires doing after the read is completed

    //emit a signal as well
    emit units_refreshed();
    emit connected();
}

void DwarfTherapist::write_settings() {

    if (m_custom_professions.size() > 0 || m_custom_prof_icns.size() > 0) {
        m_user_settings->beginGroup("custom_professions");
        m_user_settings->remove(""); // clear all of them, so we can re-write

        CustomProfession *cp;
        //save the custom professions
        foreach(cp, m_custom_professions.values()) {
            cp->save(*m_user_settings);
        }
        //save the custom profession icons
        foreach(CustomProfession *cp, m_custom_prof_icns.values()){
            cp->save(*m_user_settings);
        }
        m_user_settings->endGroup();
    }

    //save the super labors
    if (m_super_labors.size() > 0) {        
        m_user_settings->remove("super_labors"); // clear all of them, so we can re-write
        m_user_settings->beginWriteArray("super_labors");
        int idx = 0;
        foreach(SuperLabor *sl, m_super_labors) {
            m_user_settings->setArrayIndex(idx++);
            if(!sl->is_from_custom_prof())
                sl->save(*m_user_settings);
        }
        m_user_settings->endArray();
    }
    emit emit_customizations_changed();
}

QList<SuperLabor*> DwarfTherapist::get_super_labors(bool custom_profs, bool super_labors){
    if(custom_profs && super_labors){
        return m_super_labors.values();
    }else{
        QList<SuperLabor*> ret;
        foreach(SuperLabor *sl, m_super_labors.values()){
            if((custom_profs && sl->is_from_custom_prof()) || (super_labors && !sl->is_from_custom_prof()))
                ret << sl;
        }
        return ret;
    }
}

/* CUSTOMIZATIONS (custom professions, icons, super labors) */

DwarfTherapist::customization_data DwarfTherapist::build_c_data(QVariant data){
    Q_ASSERT(data.canConvert<QVariantList>());
    DwarfTherapist::customization_data c_data;
    c_data.name = data.toList().at(0).toString();
    c_data.icon_id = data.toList().at(1).toInt();
    c_data.is_super = data.toList().at(2).toBool();
    return c_data;
}

void DwarfTherapist::import_existing_professions() {
    int imported = 0;
    foreach(Dwarf *d, get_dwarves()) {
        QString prof = d->custom_profession_name();
        if (prof.isEmpty())
            continue;
        CustomProfession *cp = get_custom_profession(prof);
        if (!cp) { // import it
            cp = new CustomProfession(d, this);
            if(cp->prof_id() > -1)
                m_custom_prof_icns.insert(cp->prof_id(),cp);
            else
                m_custom_professions.insert(prof, cp);
            imported++;
        }
    }
    m_main_window->load_customizations();
    QMessageBox::information(m_main_window, tr("Import Successful"),
        tr("Imported %n custom profession(s)", "", imported));
}

CustomProfession *DwarfTherapist::get_custom_profession(QString name) {
    CustomProfession *retval = m_custom_professions.value(name,0);
    return retval;
}

void DwarfTherapist::add_custom_profession(CustomProfession *cp) {
    m_custom_professions.insert(cp->get_name(), cp);
    m_main_window->load_customizations();
    write_settings();
}

int DwarfTherapist::add_custom_profession(Dwarf *d) {
    CustomProfession *cp = new CustomProfession(d, this);
    int accepted = cp->show_builder_dialog(m_main_window);
    if (accepted) {
        m_custom_professions.insert(cp->get_name(),cp);
        m_main_window->load_customizations();
        write_settings();
    }
    return accepted;
}

//! from CP context menu's "Edit..."
void DwarfTherapist::edit_customization() {
    QAction *a = qobject_cast<QAction*>(QObject::sender());
    edit_customization(a->data().toList());
}

//! from double-clicking a profession
void DwarfTherapist::edit_customization(QTreeWidgetItem *i) {
    if(i->childCount() > 0)
        return;
    QList<QVariant> data;
    data << i->text(0) << i->data(0,Qt::UserRole) << i->data(0,Qt::UserRole+1);
    edit_customization(data);
}

void DwarfTherapist::edit_customization(QList<QVariant> data){
    DwarfTherapist::customization_data c_data = build_c_data(data);
    if(!c_data.is_super){
        CustomProfession *cp;
        if(c_data.icon_id > -1)
            cp = get_custom_prof_icon(c_data.icon_id);
        else
            cp = get_custom_profession(c_data.name);

        if (!cp) {
            LOGW << "tried to edit custom profession '" << c_data.name << "' but I can't find it!";
            return;
        }
        int accepted = cp->show_builder_dialog(m_main_window);
        if (accepted) {
            m_main_window->load_customizations();
            write_settings();            
        }
    }else{
        SuperLabor *sl;
        sl = get_super_labor(c_data.name);
        if(!sl){
            LOGW << "tried to edit super labor '" << c_data.name << "' but I can't find it!";
            return;
        }
        int accepted = sl->show_builder_dialog(m_main_window);
        if (accepted) {
            m_main_window->load_customizations();
            write_settings();            
        }
    }
}

void DwarfTherapist::delete_customization() {
    QAction *a = qobject_cast<QAction*>(QObject::sender());
    DwarfTherapist::customization_data c_data = build_c_data(a->data());

    if(!c_data.is_super){
        CustomProfession *cp;
        if(c_data.icon_id < 0)
            cp = get_custom_profession(c_data.name);
        else
            cp = get_custom_prof_icon(c_data.icon_id);

        if (!cp) {
            LOGW << "tried to delete custom profession '" << c_data.name << "' but I can't find it!";
            return;
        }

        if(c_data.icon_id < 0){ //custom profession
            QList<Dwarf*> blockers;
            foreach(Dwarf *d, m_main_window->get_model()->get_dwarves()) {
                if (d->custom_profession_name() == c_data.name) {
                    blockers << d;
                }
            }
            if (blockers.size() > 0) {
                QMessageBox *box = new QMessageBox(m_main_window);
                box->setIcon(QMessageBox::Warning);
                box->setWindowTitle(tr("Cannot Remove Profession"));
                box->setText(tr("The following %1 dwarf(s) is(are) still using <b>%2</b>. Please change them to"
                                " another profession before deleting this profession!").arg(blockers.size()).arg(c_data.name));
                QString msg = tr("Dwarfs with this profession:\n\n");
                foreach(Dwarf *d, blockers) {
                    msg += d->nice_name() + "\n";
                }
                box->setDetailedText(msg);
                box->exec();
            }else{
                //custom icon override
                cp->delete_from_disk();
                m_custom_professions.remove(cp->get_name());
                delete cp;
            }
        }else{
            cp->delete_from_disk();
            m_custom_prof_icns.remove(c_data.icon_id);
        }
    }else{
        SuperLabor *sl = get_super_labor(c_data.name);
        if (!sl) {
            LOGW << "tried to delete super labor '" << c_data.name << "' but I can't find it!";
            return;
        }
        sl->delete_from_disk();
        m_super_labors.remove(c_data.name);
    }

    m_main_window->load_customizations();
    write_settings();    
}


//! convenience method
Dwarf *DwarfTherapist::get_dwarf_by_id(int dwarf_id) {
    return m_main_window->get_model()->get_dwarf_by_id(dwarf_id);
}

void DwarfTherapist::load_game_translation_tables(DFInstance *df) {
    LOGI << "Loading language translation tables";
    qDeleteAll(m_language);
    m_language.clear();
    m_generic_words.clear();
    m_dwarf_words.clear();

    uint generic_lang_table = df->memory_layout()->address("language_vector") + df->get_memory_correction();
    uint translation_vector = df->memory_layout()->address("translation_vector") + df->get_memory_correction();
    uint word_table_offset = df->memory_layout()->offset("word_table");
    TRACE << "LANGUAGES VECTOR" << hex << translation_vector;
    TRACE << "GENERIC LANGUAGE VECTOR" << hex << generic_lang_table;
    TRACE << "WORD TABLE OFFSET" << hex << word_table_offset;

    df->attach();
    if (generic_lang_table != 0xFFFFFFFF && generic_lang_table != 0) {
        LOGI << "Loading generic strings from" << hex << generic_lang_table;
        QVector<uint> generic_words = df->enumerate_vector(generic_lang_table);
        LOGI << "generic words" << generic_words.size();
        foreach(uint word_ptr, generic_words) {
            m_generic_words << df->read_string(word_ptr);
            m_language << Word::get_word(df, word_ptr);
        }
    }

    if (translation_vector != 0xFFFFFFFF && translation_vector != 0) {
        QVector<uint> languages = df->enumerate_vector(translation_vector);
        uint dwarf_entry = 0;
        foreach(uint lang, languages) {
            QString race_name = df->read_string(lang);
            LOGI << "FOUND LANG ENTRY" << hex << lang << race_name;
            if (race_name == "DWARF")
                dwarf_entry = lang;
        }
        uint dwarf_lang_table = dwarf_entry + word_table_offset - df->VECTOR_POINTER_OFFSET;
        LOGI << "Loading dwarf strings from" << hex << dwarf_lang_table;
        QVector<uint> dwarf_words = df->enumerate_vector(dwarf_lang_table);
        LOGI << "dwarf words" << dwarf_words.size();

        foreach(uint word_ptr, dwarf_words) {
            m_dwarf_words << df->read_string(word_ptr);
        }
    }
    df->detach();
}

void DwarfTherapist::emit_customizations_changed(){
    m_main_window->get_view_manager()->redraw_current_tab();
    emit customizations_changed();
}

void DwarfTherapist::emit_settings_changed(){
    emit settings_changed();
}

void DwarfTherapist::emit_roles_changed(){
    emit roles_changed();
}

void DwarfTherapist::emit_labor_counts_updated(){
    if(m_main_window->get_DFInstance()){
        emit labor_counts_updated();
        if(get_main_window()->get_view_manager())
            get_main_window()->get_view_manager()->redraw_current_tab_headers();
    }
}

void DwarfTherapist::update_specific_header(int id, COLUMN_TYPE type){
    if(m_main_window->get_DFInstance() && get_main_window()->get_view_manager())
        get_main_window()->get_view_manager()->redraw_specific_header(id,type);
}


