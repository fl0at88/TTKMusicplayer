#include "musicdownloadquerybdalbumthread.h"
#include "musictime.h"
#///QJson import
#include "qjson/parser.h"

MusicDownLoadQueryBDAlbumThread::MusicDownLoadQueryBDAlbumThread(QObject *parent)
    : MusicDownLoadQueryAlbumThread(parent)
{
    m_queryServer = "Baidu";
}

QString MusicDownLoadQueryBDAlbumThread::getClassName()
{
    return staticMetaObject.className();
}

void MusicDownLoadQueryBDAlbumThread::startToSearch(const QString &album)
{
    if(!m_manager)
    {
        return;
    }

    M_LOGGER_INFO(QString("%1 startToSearch %2").arg(getClassName()).arg(album));
    QUrl musicUrl = MusicUtils::Algorithm::mdII(BD_ALBUM_URL, false).arg(album);
    deleteAll();

    QNetworkRequest request;
    request.setUrl(musicUrl);
    request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
#ifndef QT_NO_SSL
    QSslConfiguration sslConfig = request.sslConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(sslConfig);
#endif
    m_reply = m_manager->get( request );
    connect(m_reply, SIGNAL(finished()), SLOT(downLoadFinished()));
    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(replyError(QNetworkReply::NetworkError)));
}

void MusicDownLoadQueryBDAlbumThread::downLoadFinished()
{
    if(!m_reply || !m_manager)
    {
        deleteAll();
        return;
    }

    M_LOGGER_INFO(QString("%1 downLoadFinished").arg(getClassName()));
    emit clearAllItems();      ///Clear origin items
    m_musicSongInfos.clear();  ///Empty the last search to songsInfo

    if(m_reply->error() == QNetworkReply::NoError)
    {
        QByteArray bytes = m_reply->readAll(); ///Get all the data obtained by request

        QJson::Parser parser;
        bool ok;
        QVariant data = parser.parse(bytes, &ok);
        if(ok)
        {
            QVariantMap value = data.toMap();
            if(value.contains("albumInfo") && value.contains("songlist"))
            {
                bool albumFlag = false;
                QVariantMap albumInfo = value["albumInfo"].toMap();
                MusicPlaylistItem info;
                info.m_coverUrl = albumInfo["pic_small"].toString().replace(",w_90", ",w_500");
                info.m_description = albumInfo["title"].toString() + "<>" +
                                     albumInfo["language"].toString() + "<>" +
                                     albumInfo["publishcompany"].toString() + "<>" +
                                     albumInfo["publishtime"].toString();
                ////////////////////////////////////////////////////////////
                QVariantList datas = value["songlist"].toList();
                foreach(const QVariant &var, datas)
                {
                    if(var.isNull())
                    {
                        continue;
                    }

                    value = var.toMap();
                    MusicObject::MusicSongInformation musicInfo;
                    musicInfo.m_singerName = value["author"].toString();
                    musicInfo.m_songName = value["title"].toString();
                    musicInfo.m_timeLength = MusicTime::msecTime2LabelJustified(value["file_duration"].toString().toInt()*1000);

                    musicInfo.m_songId = value["song_id"].toString();
                    musicInfo.m_albumId = value["album_id"].toString();
                    musicInfo.m_artistId = value["ting_uid"].toString();
                    musicInfo.m_lrcUrl = value["lrclink"].toString();
                    musicInfo.m_smallPicUrl = value["pic_small"].toString().replace(",w_90", ",w_500");
                    musicInfo.m_albumName = value["album_title"].toString();

                    if(!m_manager || m_stateCode != MusicNetworkAbstract::Init) return;
                    readFromMusicSongAttribute(&musicInfo, value["all_rate"].toString(), m_searchQuality, m_queryAllRecords);
                    if(!m_manager || m_stateCode != MusicNetworkAbstract::Init) return;

                    if(musicInfo.m_songAttrs.isEmpty())
                    {
                        continue;
                    }
                    ////////////////////////////////////////////////////////////
                    if(!albumFlag)
                    {
                        albumFlag = true;
                        info.m_id = musicInfo.m_albumId;
                        info.m_name = musicInfo.m_singerName;
                        emit createAlbumInfoItem(info);
                    }
                    ////////////////////////////////////////////////////////////
                    MusicSearchedItem item;
                    item.m_songName = musicInfo.m_songName;
                    item.m_singerName = musicInfo.m_singerName;
                    item.m_albumName = musicInfo.m_albumName;
                    item.m_time = musicInfo.m_timeLength;
                    item.m_type = mapQueryServerString();
                    emit createSearchedItems(item);
                    m_musicSongInfos << musicInfo;
                }
            }
        }
    }

    emit downLoadDataChanged(QString());
    deleteAll();
    M_LOGGER_INFO(QString("%1 downLoadFinished deleteAll").arg(getClassName()));
}
