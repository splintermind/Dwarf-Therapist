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
#ifndef UTILS_H
#define UTILS_H

#include <QByteArray>
#include <QColor>
#include <QtGlobal>
#include <QVariant>
#include <QPixmap>
#include <QBuffer>
#include <QIODevice>
#include <math.h>

// valid for as long as DF stays 32bit
typedef quint32 VIRTADDR;
typedef quint8 BYTE;
typedef quint16 WORD;

static inline QByteArray encode_short(short num) {
    char *bytes;
    bytes = (char*)&num;
    QByteArray arr(bytes, sizeof(short));
    return arr;
}

static inline QByteArray encode(int num) {
    char *bytes;
    bytes = (char*)&num;
    QByteArray arr(bytes, sizeof(int));
    return arr;
}

static inline QByteArray encode(VIRTADDR num) {
    char *bytes;
    bytes = (char*)&num;
    QByteArray arr(bytes, sizeof(uint));
    return arr;
}

static inline QByteArray encode(const ushort &num) {
    char *bytes;
    bytes = (char*)&num;
    QByteArray arr(bytes, sizeof(ushort));
    return arr;
}

static inline BYTE decode_byte(const QByteArray &arr) {
    BYTE *out_ptr = (BYTE*)arr.constData();
    return *out_ptr;
}

static inline WORD decode_word(const QByteArray &arr) {
    WORD *out_ptr = (WORD*)arr.constData();
    return *out_ptr;
}

static inline quint32 decode_dword(const QByteArray &arr) {
    quint32 *out_ptr = (quint32*)arr.constData();
    return *out_ptr;
}

static inline int decode_int(const QByteArray &arr) {
    const char* data = arr.constData();
    int *out_ptr = (int*)data;
    return *out_ptr;
}

static inline short decode_short(const QByteArray &arr) {
    short *out_ptr = (short*)arr.constData();
    return *out_ptr;
}

static inline QByteArray encode_skillpattern(short skill, short exp, short rating) {
    QByteArray bytes;
    bytes.reserve(6);
    bytes[0] = (uchar)skill;
    bytes[1] = (uchar)(skill >> 8);
    bytes[2] = (uchar)exp;
    bytes[3] = (uchar)(exp >> 8);
    bytes[4] = (uchar)rating;
    bytes[5] = (uchar)(rating >> 8);
    return bytes;
}

static inline QString by_char(QByteArray arr) {
    QString out;
    for(int i=0; i < arr.size(); i++) {
        out += QString::number((uchar)arr.at(i), 16);
        out += " ";
    }
    return out;
}

static inline QColor compliment(const QColor &in_color, bool true_compliment = false, float brighness_threshold = 0.50) {
    if(!true_compliment){
        //    qreal brightness = (in_color.red() * 299 + in_color.green() * 587 + in_color.blue() * 114) / 255000.0;
        qreal brightness = sqrt(pow(in_color.redF(),2.0) * 0.241 +
                                pow(in_color.greenF(),2.0) * 0.691 +
                                pow(in_color.blueF(),2.0) * 0.068);
        QColor tmp = in_color.toHsv();
        int h = tmp.hue();
        int s = 25;
        int v;
        if(brightness >= brighness_threshold || in_color.alpha() < 130) {
            v = 0;
        } else {
            v = 255;
        }
        return QColor::fromHsv(h, s, v);
    }else{
            int r = (~in_color.red()) & 0xff;
            int b = (~in_color.blue()) & 0xff;
            int g = (~in_color.green()) & 0xff;
//            int a = in_color.alpha(); //leave the alpha the same
        //    int a = (~in_color.alpha()) & 0xff;
            return QColor(r,g,b,255);
    }
}

static inline QColor from_hex(const QString &h) {
    bool ok;
    QColor retval = Qt::gray;
    if (h.length() == 8) { // "0x99AABB" (no alpha)
        retval = QColor(h.toInt(&ok, 16));
    } else if (h.length() == 10) { // "0x99AABBFF" (last two for alpha channel)
        int r = h.mid(2, 2).toInt(&ok, 16);
        int g = h.mid(4, 2).toInt(&ok, 16);
        int b = h.mid(6, 2).toInt(&ok, 16);
        int a = h.mid(8, 2).toInt(&ok, 16);
        retval = QColor(r, g, b, a);
    }
    return retval;
}

static inline QString to_hex(const QColor &c) {
    return QString("0x%1%2%3%4")
        .arg(c.red(), 2, 16, QChar('0'))
        .arg(c.green(), 2, 16, QChar('0'))
        .arg(c.blue(), 2, 16, QChar('0'))
        .arg(c.alpha(), 2, 16, QChar('0'));
}
static inline QString hexify(const uint &num) {
    return QString("0x%1").arg(num, 8, 16, QChar('0'));
}

static inline QString capitalize(const QString & word) {
    QString result = word;
    if(!result.isEmpty()) {
        result = result.toLower();
        result[0] = result[0].toUpper();
    }
    return result;
}

static inline QString capitalizeEach(const QString & word){
    QString result = word;
    QStringList list = result.split(" ");
    for(int i=0; i<list.length(); i++){
        list[i] = capitalize(list[i]);
    }
    result = list.join(" ");
    return result;
}

template <class T> class vPtr
{
public:
    static T* asPtr(QVariant v){
        return (T*)v.value<void *>();
    }
    static QVariant asQVariant(T* ptr){
        return qVariantFromValue((void*) ptr);
    }
};

static inline QString embedPixmap(const QPixmap &img){
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    img.save(&buffer, "PNG");
    return QString("<img src=\"data:image/png;base64,%1\"/>").arg(QString(buffer.data().toBase64()));
}

static inline bool has_flag(int flag, int flags){
    return ((flag & flags) == flag);
}

static inline QString nice_list(QStringList values){
    QString ret_val = "";
    QString last_msg;
    if(values.count() > 1)
        last_msg = values.takeLast();
    ret_val = values.join(", ");
    if(!last_msg.isEmpty()){
        ret_val.append(QObject::tr(" and ") + last_msg);
    }
    return ret_val;
}

#endif // UTILS_H
