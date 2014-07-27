#include "laborlistbase.h"
#include "labor.h"
#include "dwarf.h"
#include "dwarftherapist.h"

LaborListBase::LaborListBase(QObject *parent)
    :QObject(parent)
    , m_dwarf(0)
    , gdr(GameDataReader::ptr())
    , m_name("")
    , m_role_name("")
    , m_dialog(0)
    , m_selected_count(0)
    , m_internal_change_flag(false)
{    
    //connect(DT,SIGNAL(customizations_changed()),this,SLOT(refresh()),Qt::UniqueConnection); //refresh after any editing
    connect(DT,SIGNAL(roles_changed()),this,SLOT(refresh()),Qt::UniqueConnection); //refresh after any role changes
    connect(DT,SIGNAL(units_refreshed()),this,SLOT(refresh()),Qt::UniqueConnection); //refresh information after a read
    connect(DT, SIGNAL(settings_changed()), this, SLOT(read_settings()));
    read_settings();
}
LaborListBase::~LaborListBase(){
    gdr = 0;
    m_dwarf = 0;
}

/*!
Get a vector of all enabled labors in this template by labor_id
*/
QVector<int> LaborListBase::get_enabled_labors() {
    QVector<int> labors;
    foreach(int labor, m_active_labors.uniqueKeys()) {
        if (m_active_labors.value(labor)) {
            labors << labor;
        }
    }
    return labors;
}

/*!
Check if the template has a labor enabled

\param[in] labor_id The id of the labor to check
\returns true if this labor is enabled
*/
bool LaborListBase::is_active(int labor_id) {
    return m_active_labors.value(labor_id, false);
}

void LaborListBase::labor_item_check_changed(QListWidgetItem *item) {
    m_internal_change_flag = true;
    if (item->checkState() == Qt::Checked) {
        item->setBackgroundColor(m_active_labor_col);
        add_labor(item->data(Qt::UserRole).toInt());
        if(!m_internal_change_flag)
            m_selected_count++;
    } else {
        item->setBackground(QBrush());
        remove_labor(item->data(Qt::UserRole).toInt());
        if(!m_internal_change_flag)
            m_selected_count--;
    }
    emit selected_count_changed(m_selected_count);
    m_internal_change_flag = false;
}

void LaborListBase::load_labors(QListWidget *labor_list){
    m_selected_count = 0;
    read_settings();
    QList<Labor*> labors = gdr->get_ordered_labors();
    foreach(Labor *l, labors) {
        QListWidgetItem *item = new QListWidgetItem(l->name, labor_list);
        item->setData(Qt::UserRole, l->labor_id);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        if (is_active(l->labor_id)) {
            item->setCheckState(Qt::Checked);
            item->setBackgroundColor(m_active_labor_col);
            m_selected_count++;
        } else {
            item->setCheckState(Qt::Unchecked);
            item->setBackground(QBrush());
        }
        labor_list->addItem(item);
    }    
    connect(labor_list,SIGNAL(itemChanged(QListWidgetItem*)),this,SLOT(labor_item_check_changed(QListWidgetItem*)),Qt::UniqueConnection);
    emit selected_count_changed(m_selected_count);
}

void LaborListBase::build_role_combo(QComboBox *cb_roles){
    //add roles
    cb_roles->addItem("None (Role Average)", "");
    QPair<QString,Role*> role_pair;
    foreach(role_pair, gdr->get_ordered_roles()){
        cb_roles->addItem(role_pair.first, role_pair.first);
    }
    int index = cb_roles->findText(m_role_name);
    if(index != -1)
        cb_roles->setCurrentIndex(index);
    else
        cb_roles->setCurrentIndex(0);
}


void LaborListBase::set_labor(int labor_id, bool active) {
    if (m_active_labors.contains(labor_id) && !active)
        m_active_labors.remove(labor_id);
    if (active)
        m_active_labors.insert(labor_id, true);
}

void LaborListBase::refresh(){
    m_qvariant_labors.clear();
    m_ratings.clear();
    m_labor_desc.clear();

    QList<Dwarf*> dwarves = DT->get_dwarves();

    QVector<int> labors = get_enabled_labors();
    foreach(int labor_id, labors){
        Labor *l = GameDataReader::ptr()->get_labor(labor_id);
        QString name = l->name;
        m_labor_desc.insert(l->labor_id,name);
        m_qvariant_labors.append(labor_id);
        foreach(Dwarf *d, dwarves){

            QList<float> ratings = m_ratings.take(d->id());
            if(ratings.size() <= 0)
                ratings << 0 << 0 << 0 << 0;

            if(d->labor_enabled(l->labor_id))
                ratings[LLB_ACTIVE] += 1;
            ratings[LLB_SKILL] += d->skill_level(l->skill_id,true,true);
            ratings[LLB_SKILL_RATE] += d->get_skill(l->skill_id).skill_rate();
            if(m_role_name.isEmpty()){
                //find related roles based on the labor's skill
                QVector<Role*> roles = gdr->get_skill_roles().value(l->skill_id);
                if(roles.size() > 0){
                    ratings[LLB_ROLE] += d->get_role_rating(roles.first()->name,true);
                }
            }else if(!m_ratings.contains(d->id())){
                ratings[LLB_ROLE] = d->get_role_rating(m_role_name,true);
            }
            m_ratings.insert(d->id(),ratings);
        }
    }
    foreach(Dwarf *d, dwarves){
        m_ratings[d->id()][LLB_SKILL] /= labors.count();
        m_ratings[d->id()][LLB_SKILL_RATE] /= labors.count();
        if(m_role_name == "")
            m_ratings[d->id()][LLB_ROLE] /= labors.count();
        if(m_active_labors.count() > 0)
            m_ratings[d->id()][LLB_ACTIVE] = (1000.0f * ((float)m_ratings[d->id()][LLB_ACTIVE]/(float)m_active_labors.count()));
    }
    read_settings();
}

float LaborListBase::get_rating(int id, LLB_RATING_TYPE type){
    if(m_ratings.size() <= 0 && get_enabled_labors().size() > 0)
        refresh();
    if(m_ratings.contains(id)){
        QList<float> ratings = m_ratings.value(id);
        if(int(type) < 0 || int(type) > ratings.count()-1){
            LOGW << "out of" << ratings.count() << "ratings, rating type" << (int)type << "could not be found!";
            if(ratings.count() > 0)
                return ratings.at(0);
            else
                return 0.0;
        }else{
            return ratings.at(type);
        }
    }else{
        return 0.0;
    }
}

void LaborListBase::set_name(QString name){
    m_name = name;
}

/*!
Called when the show_builder_dialog widget's OK button is pressed, or the
dialog is otherwise accepted by the user

We intercept this call to verify the form is valid before saving it.
\sa is_valid()
*/
void LaborListBase::accept() {
    if (!is_valid()) {
        return;
    }
    if(m_dwarf){
        update_dwarf();
        m_dwarf = 0;
    }
    refresh();
    m_dialog->accept();
}

void LaborListBase::cancel(){
    m_dwarf = 0;
    m_dialog->reject();
}

void LaborListBase::read_settings(){
    QSettings *s = DT->user_settings();
    if(s)
        m_active_labor_col = s->value("options/colors/active_labor",QColor(Qt::white)).value<QColor>();
}
