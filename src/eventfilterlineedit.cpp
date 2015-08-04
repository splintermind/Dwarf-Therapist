#include <QKeyEvent>
#include <QLineEdit>
#include <QAbstractItemView>
#include "eventfilterlineedit.h"
#include "truncatingfilelogger.h"

bool EventFilterLineEdit::eventFilter(QObject* watched, QEvent* event)
{
    QAbstractItemView* view = qobject_cast<QAbstractItemView*>(watched);
    //LOGI << event->type();
    //event->type() == QEvent::HideToParent || event->type() == QEvent::Hide || event->type() == QEvent::Leave ||
//        if(event->type() == QEvent::Leave){
//            mLineEdit->clear();
//            view->hide();
//            return true;
//        }
    if (event->type() == QEvent::KeyPress){
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return ||
                keyEvent->key() == Qt::Key_Enter)
        {
            mLineEdit->clear();
            view->hide();
            emit enterPressed(view->currentIndex());
            return true;
        }
    }
    return false;
}
