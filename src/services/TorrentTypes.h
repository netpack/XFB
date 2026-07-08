#ifndef TORRENTTYPES_H
#define TORRENTTYPES_H

#include <QString>
#include <QMetaType>

struct TorrentSearchResult {
    QString name;
    QString magnetLink;
    QString torrentUrl;
    QString size;
    QString seeders;
    QString leechers;
    QString uploadDate;
    QString uploader;
    bool isAudio;
    bool isTrusted;
    QString category;
};

Q_DECLARE_METATYPE(TorrentSearchResult)

#endif // TORRENTTYPES_H
