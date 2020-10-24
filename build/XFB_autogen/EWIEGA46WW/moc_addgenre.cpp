/****************************************************************************
** Meta object code from reading C++ file 'addgenre.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.11.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../addgenre.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'addgenre.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.11.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_addgenre_t {
    QByteArrayData data[9];
    char stringdata0[137];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_addgenre_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_addgenre_t qt_meta_stringdata_addgenre = {
    {
QT_MOC_LITERAL(0, 0, 8), // "addgenre"
QT_MOC_LITERAL(1, 9, 22), // "on_btShowGenre_clicked"
QT_MOC_LITERAL(2, 32, 0), // ""
QT_MOC_LITERAL(3, 33, 21), // "on_btDelGenre_clicked"
QT_MOC_LITERAL(4, 55, 21), // "on_listGenres_clicked"
QT_MOC_LITERAL(5, 77, 11), // "QModelIndex"
QT_MOC_LITERAL(6, 89, 5), // "index"
QT_MOC_LITERAL(7, 95, 24), // "on_btAddNewGenre_clicked"
QT_MOC_LITERAL(8, 120, 16) // "on_close_clicked"

    },
    "addgenre\0on_btShowGenre_clicked\0\0"
    "on_btDelGenre_clicked\0on_listGenres_clicked\0"
    "QModelIndex\0index\0on_btAddNewGenre_clicked\0"
    "on_close_clicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_addgenre[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   39,    2, 0x08 /* Private */,
       3,    0,   40,    2, 0x08 /* Private */,
       4,    1,   41,    2, 0x08 /* Private */,
       7,    0,   44,    2, 0x08 /* Private */,
       8,    0,   45,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 5,    6,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void addgenre::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        addgenre *_t = static_cast<addgenre *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->on_btShowGenre_clicked(); break;
        case 1: _t->on_btDelGenre_clicked(); break;
        case 2: _t->on_listGenres_clicked((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 3: _t->on_btAddNewGenre_clicked(); break;
        case 4: _t->on_close_clicked(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject addgenre::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_addgenre.data,
      qt_meta_data_addgenre,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *addgenre::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *addgenre::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_addgenre.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int addgenre::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 5;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
