//=================================================================================================
/*
    Copyright (C) 2015-2017 Sky kit authors united with omega. <http://omega.gg/about>

    Author: Benjamin Arnaud. <http://bunjee.me> <bunjee@omega.gg>

    This file is part of the SkTorrent module of Sky kit.

    - GNU General Public License Usage:
    This file may be used under the terms of the GNU General Public License version 3 as published
    by the Free Software Foundation and appearing in the LICENSE.md file included in the packaging
    of this file. Please review the following information to ensure the GNU General Public License
    requirements will be met: https://www.gnu.org/licenses/gpl.html.
*/
//=================================================================================================

#include "WTorrentEngine.h"

#ifndef SK_NO_TORRENTENGINE

// Qt includes
#include <QCoreApplication>
#include <QDir>
#ifdef QT_LATEST
#include <QDataStream>
#endif

// libtorrent includes
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/bencode.hpp>
#include <libtorrent/create_torrent.hpp>
#include <libtorrent/extensions/ut_pex.hpp>

// Boost includes
#include <boost/bind.hpp>

// C++ includes
#include <fstream>

// Sk includes
#include <WControllerApplication>
#include <WControllerFile>
#include <WControllerNetwork>

// Metatypes
Q_DECLARE_METATYPE(torrent_handle)

//-------------------------------------------------------------------------------------------------
// Defines

#if LIBTORRENT_VERSION_NUM > 10100
#define LIBTORRENT_LATEST
#endif

//-------------------------------------------------------------------------------------------------
// Static variables

static const int TORRENTENGINE_BLOCK = 16 * 1024; // 16 bytes

static const int TORRENTENGINE_PRIORITY_COUNT    =   3;
static const int TORRENTENGINE_PRIORITY_INTERVAL = 100;

static const int TORRENTENGINE_INTERVAL        = 1000;
static const int TORRENTENGINE_INTERVAL_REMOVE =  100;

//-------------------------------------------------------------------------------------------------
// Private
//-------------------------------------------------------------------------------------------------

WTorrentEnginePrivate::WTorrentEnginePrivate(WTorrentEngine * p) : WPrivate(p) {}

//-------------------------------------------------------------------------------------------------

void WTorrentEnginePrivate::init(const QString & path, qint64 sizeMax, QThread * thread)
{
    Q_Q(WTorrentEngine);

    session = NULL;

    this->path = path + '/';

    this->sizeMax = sizeMax;

    _sizeMax = sizeMax;

    if (thread) q->moveToThread(thread);

    QCoreApplication::postEvent(q, new QEvent(static_cast<QEvent::Type>
                                              (WTorrentEnginePrivate::EventCreate)),
                                Qt::HighEventPriority * 100);
}

//-------------------------------------------------------------------------------------------------
// Private functions
//-------------------------------------------------------------------------------------------------

void WTorrentEnginePrivate::load()
{
    QFile file(pathIndex);

    if (file.exists() == false) return;

    if (file.open(QIODevice::ReadOnly) == false)
    {
        qWarning("WTorrentEnginePrivate::load: Failed to open index %s.", pathIndex.C_STR);

        return;
    }

    QDataStream stream(&file);

    int count;

    stream >> count;

    while (count)
    {
        int id;

        QString hash;

        qint64 size;

        int length;

        QList<QUrl> urls;

        stream >> id >> hash >> size >> length;

        while (length)
        {
            QUrl url;

            stream >> url;

            urls.append(url);

            length--;
        }

        ids.insertId(id);

        WTorrentSource * source = new WTorrentSource;

        source->id   = id;
        source->hash = sha1_hash(hash.toStdString());
        source->size = size;
        source->urls = urls; // FIXME

        sources.append(source);

        this->size += size;

        count--;
    }
}

void WTorrentEnginePrivate::save() const
{
    if (timerSave->isActive()) return;

    timerSave->start();
}

//-------------------------------------------------------------------------------------------------

torrent_info * WTorrentEnginePrivate::loadInfo(const QByteArray & array) const
{
    const char * data = array.data();

    bdecode_node node;

    error_code error;

    if (bdecode(data, data + array.size(), node, error) == 0)
    {
         return new torrent_info(node);
    }
    else return NULL;
}

void WTorrentEnginePrivate::loadResume(WTorrentData * data, const QString & fileName) const
{
    QFile file(fileName);

    if (file.open(QIODevice::ReadOnly) == false)
    {
        qWarning("WTorrentEnginePrivate::loadResume: Failed to open file %s.", fileName.C_STR);

        return;
    }

    QString content = Sk::readAscii(file.readAll());

    int index = WControllerTorrent::indexAfter(content, "pieces");

    QString finished = WControllerTorrent::extractString(content, index);

    index = WControllerTorrent::indexAfter(content, "unfinished");

    QString unfinished = WControllerTorrent::extractList(content, index);

    qDebug("BLOCK COUNT %d %d [%s] [%s]",
           finished.length(), unfinished.length(), finished.C_STR, unfinished.C_STR);

    QBitArray * pieces = &(data->pieces);

    if (finished.length() == pieces->count())
    {
        const char * bits = finished.C_STR;

        for (int i = 0; i < finished.length(); i++)
        {
            if (*bits & 1)
            {
                qDebug("PIECE %d", i);

                pieces->setBit(i);
            }

            bits++;
        }
    }

    if (unfinished.length() == 1) return;

    QBitArray * blocks = &(data->blocks);

    QStringList list = WControllerTorrent::splitList(unfinished);

    foreach (const QString & string, list)
    {
        int block = WControllerTorrent::integerAfter(string, "piece") * data->blockCount;

        QString bitmask = WControllerTorrent::stringAfter(string, "bitmask");

        const char * bits = bitmask.C_STR;

        for (int i = 0; i < bitmask.length(); i++)
        {
            char character = *bits;

            for (int j = 0; j < 8; j++)
            {
                if (character & 1)
                {
                    qDebug("BLOCK %d", block);

                    blocks->setBit(block);
                }

                character >>= 1;

                block++;
            }

            bits++;
        }

        qDebug("ENTRY %d %s", block, bitmask.C_STR);
    }
}

//-------------------------------------------------------------------------------------------------

WTorrentData * WTorrentEnginePrivate::createData(TorrentInfoPointer info, const sha1_hash & hash,
                                                                          const QUrl      & url)
{
    WTorrentData * data = new WTorrentData;

    int count = info->num_pieces();

    int blockCount = info->piece_length() / TORRENTENGINE_BLOCK;

    data->blockCount = blockCount;

    data->pieces = QBitArray(count);
    data->blocks = QBitArray(count * blockCount);

    WTorrentSource * source = getSource(hash);

    QString path;

    add_torrent_params params;

    if (source)
    {
        QString number = QString::number(source->id);

        path = this->path + number;

        QString fileName = path + "/." + number;

        loadResume(data, fileName);

        std::ifstream stream(fileName.C_STR, std::ios_base::binary);

        stream.unsetf(std::ios_base::skipws);

        params.resume_data.assign(std::istream_iterator<char>(stream),
                                  std::istream_iterator<char>());

        QList<QUrl> * urls = &(source->urls);

        if (urls->contains(url) == false)
        {
            urls->append(url);

            save();
        }
    }
    else
    {
        source = new WTorrentSource;

        int id = ids.generateId();

        source->id   = id;
        source->hash = info->info_hash();
        source->size = 0;

        source->urls.append(url);

        sources.append(source);

        path = this->path + QString::number(id);

        save();
    }

    int fileCount = info->num_files();

    data->source = source;

    data->path = path;

    data->fileCount = fileCount;

    data->hash = 0;

    data->files = std::vector<int>(fileCount, 0);

    datas.append(data);

    //---------------------------------------------------------------------------------------------

    params.ti        = info;
    params.save_path = path.toStdString();

    session->async_add_torrent(params);

    return data;
}

//-------------------------------------------------------------------------------------------------

void WTorrentEnginePrivate::updateData(WTorrentData * data)
{
    WTorrentSource * source = data->source;

    sources.removeOne(source);
    sources.   append(source);

    QHashIterator<QTimer *, WTorrentData *> i(deleteTorrents);

    while (i.hasNext())
    {
        i.next();

        if (i.value() == data)
        {
            qDebug("TORRENT REMOVE TIMER");

            QTimer * timer = i.key();

            deleteTorrents.remove(timer);

            delete timer;

            return;
        }
    }
}

void WTorrentEnginePrivate::removeData(WTorrentData * data)
{
    Q_Q(WTorrentEngine);

    QTimer * timer = new QTimer;

    timer->setInterval(TORRENTENGINE_INTERVAL_REMOVE);

    timer->setSingleShot(true);

    deleteTorrents.insert(timer, data);

    QObject::connect(timer, SIGNAL(timeout()), q, SLOT(onRemove()));

    timer->start();
}

//-------------------------------------------------------------------------------------------------

WTorrentItem * WTorrentEnginePrivate::createItem(TorrentInfo info, WTorrentData * data,
                                                                   WTorrent     * torrent,
                                                                   int            index,
                                                                   WTorrent::Mode mode) const
{
    QString fileName;

    QStringList paths;

    qint64 size;

    int begin;
    int end;

    if (index == -1)
    {
        const file_storage & storage = info->files();

        QString filePath = data->path + '/';

        for (int i = 0; i < data->fileCount; i++)
        {
            QString fileName = extractFileName(storage.file_path(i), i);

            paths.push_back(filePath + fileName);
        }

        size = info->total_size();

        begin = 0;
        end   = info->num_pieces();
    }
    else if (index < data->fileCount)
    {
        const file_storage & storage = info->files();

        fileName = data->path + '/' + extractFileName(storage.file_path(index), index);

        size = storage.file_size(index);

        peer_request request = info->map_file(index, 0, 0);

        begin = request.piece;

        int length = (request.start + size) / info->piece_length();

        end = begin + qMax(0, length) + 1;
    }
    else
    {
        size = 0;

        begin = 0;
        end   = 0;
    }

    WTorrentItem * item = new WTorrentItem;

    item->data = data;

    item->torrent = torrent;

    item->index = index;

    item->mode = mode;

    item->fileName = fileName;

    item->paths = paths;

    item->size = size;

    item->begin = begin;
    item->end   = end;

    data->items.append(item);

    //---------------------------------------------------------------------------------------------

    QBitArray * pieces = &(data->pieces);

    int current = begin;

    while (current < end && pieces->at(current))
    {
        current++;
    }

    item->current = current;

    if (current == end)
    {
        qDebug("TORRENT ALREADY FINISHED");

        item->finished = true;

        QCoreApplication::postEvent(torrent, new WTorrentEventAdd(paths, size));

        QCoreApplication::postEvent(torrent, new WTorrentEvent(WTorrent::EventFinished));
    }
    else
    {
        qDebug("TORRENT START AT %d", current);

        item->finished = false;

        QCoreApplication::postEvent(torrent, new WTorrentEventAdd(paths, size));
    }

    return item;
}

WTorrentStream * WTorrentEnginePrivate::createStream(TorrentInfo info, WTorrentData * data,
                                                                       WTorrent     * torrent,
                                                                       int            index,
                                                                       WTorrent::Mode mode) const
{
    QStringList paths;

    qint64 size;

    int begin;
    int end;

    QString fileName;

    int sizePiece;

    qint64 start;

    if (index == -1) index = 0;

    if (index < data->fileCount)
    {
        const file_storage & storage = info->files();

        fileName = data->path + '/' + extractFileName(storage.file_path(index), index);

        paths.append(fileName);

        //-----------------------------------------------------------------------------------------

        size = storage.file_size(index);

        peer_request request = info->map_file(index, 0, 0);

        begin = request.piece;

        sizePiece = info->piece_length();

        start = request.start;

        int length = (start + size) / sizePiece;

        end = begin + qMax(0, length) + 1;
    }
    else
    {
        size = 0;

        begin = 0;
        end   = 0;

        sizePiece = 0;

        start = 0;
    }

    int count = end - begin;

    WTorrentStream * stream = new WTorrentStream;

    stream->data = data;

    stream->torrent = torrent;

    stream->index = index;

    stream->mode = mode;

    stream->fileName = fileName;

    stream->paths = paths;

    stream->size = size;

    stream->begin = begin;
    stream->end   = end;

    //---------------------------------------------------------------------------------------------

    stream->sizePiece = sizePiece;

    stream->count = count;

    stream->start = start;

    stream->position = 0;

    data->items.append(stream);

    //---------------------------------------------------------------------------------------------

    QBitArray * pieces = &(data->pieces);

    int current = begin;

    while (current < end && pieces->at(current))
    {
        current++;
    }

    stream->piece   = current - begin;
    stream->current = current;

    if (current == end)
    {
        qDebug("TORRENT ALREADY FINISHED");

        stream->finished = true;

        stream->block = 0;

        stream->bufferPieces = size;
        stream->bufferBlocks = size;

        QCoreApplication::postEvent(torrent, new WTorrentEventAdd(paths, size));

        QCoreApplication::postEvent(torrent, new WTorrentEventBuffer(size, size));

        QCoreApplication::postEvent(torrent, new WTorrentEvent(WTorrent::EventFinished));
    }
    else
    {
        stream->finished = false;

        QBitArray * blocks = &(data->blocks);

        int block      = 0;
        int blockCount = data->blockCount;

        int blockCurrent = current * blockCount;

        while (block < blockCount && blocks->at(blockCurrent))
        {
            block       ++;
            blockCurrent++;
        }

        stream->block = block;

        qDebug("TORRENT START AT %d %d", stream->piece, block);

        qint64 bufferPieces = (qint64) stream->piece * (qint64) stream->sizePiece - stream->start;

        qint64 bufferBlocks = bufferPieces + (qint64) block * (qint64) TORRENTENGINE_BLOCK;

        if (bufferPieces > 0)
        {
             stream->bufferPieces = bufferPieces;
        }
        else stream->bufferPieces = 0;

        if (bufferBlocks > 0)
        {
             stream->bufferBlocks = bufferBlocks;

             QCoreApplication::postEvent(torrent, new WTorrentEventAdd(paths, size));

             QCoreApplication::postEvent(torrent, new WTorrentEventBuffer(stream->bufferPieces,
                                                                          bufferBlocks));
        }
        else
        {
            stream->bufferBlocks = 0;

            QCoreApplication::postEvent(torrent, new WTorrentEventAdd(paths, size));
        }
    }

    return stream;
}

//-------------------------------------------------------------------------------------------------

void WTorrentEnginePrivate::addItem(const torrent_handle & handle, WTorrentItem * item) const
{
    if (item->finished) return;

    if (item->mode == WTorrent::Sequential)
    {
        handle.set_sequential_download(true);
    }
}

void WTorrentEnginePrivate::addStream(const torrent_handle & handle, WTorrentStream * stream)
{
    if (stream->finished) return;

    //---------------------------------------------------------------------------------------------
    // Deadline

    int current = stream->begin;

    int priority = TORRENTENGINE_PRIORITY_COUNT;
    int deadline = TORRENTENGINE_PRIORITY_INTERVAL;

    while (priority && current < stream->end)
    {
        handle.set_piece_deadline(current, deadline);

        deadline += TORRENTENGINE_PRIORITY_INTERVAL;

        current++;

        priority--;
    }

    //---------------------------------------------------------------------------------------------
    // Position

    qint64 position = stream->position;

    if (position)
    {
        prioritize(handle, stream, position);
    }
}

//-------------------------------------------------------------------------------------------------
// Files

void WTorrentEnginePrivate::selectFile(WTorrentItem * item) const
{
    if (item->finished) return;

    WTorrentData * data = item->data;

    std::vector<int> & files = data->files;

    int index = item->index;

    if (index != -1)
    {
        if (data->items.count() == 1)
        {
            std::fill(files.begin(), files.end(), 0);
        }

        qDebug("TORRENT INDEX %d", index);

        files[index] = 1;
    }
    else std::fill(files.begin(), files.end(), 1);

    data->handle.prioritize_files(files);

    data->handle.resume();
}

void WTorrentEnginePrivate::unselectFile(WTorrentItem * item) const
{
    if (item->finished) return;

    int index = item->index;

    if (index != -1)
    {
        WTorrentData * data = item->data;

        foreach (WTorrentItem * item, data->items)
        {
            if (item->index == index) return;
        }

        std::vector<int> & files = data->files;

        files[index] = 0;

        data->handle.prioritize_files(files);
    }
    else updateFiles(item->data);
}

//-------------------------------------------------------------------------------------------------

void WTorrentEnginePrivate::updateFiles(WTorrentData * data) const
{
    qDebug("TORRENT UPDATE FILES");

    std::vector<int> & files = data->files;

    foreach (WTorrentItem * item, data->items)
    {
        int index = item->index;

        if (index == -1)
        {
            std::fill(files.begin(), files.end(), 1);

            data->handle.prioritize_files(files);

            return;
        }
        else if (item->finished == false)
        {
            files[index] = 1;
        }
    }

    data->handle.prioritize_files(files);
}

//-------------------------------------------------------------------------------------------------

void WTorrentEnginePrivate::renameFiles(WTorrentData * data, const torrent_handle & handle) const
{
    foreach (WTorrentItem * item, data->items)
    {
        renameFile(handle, item);
    }
}

void WTorrentEnginePrivate::renameFile(const torrent_handle & handle, WTorrentItem * item) const
{
    QString fileName = item->fileName;

    if (fileName.isEmpty())
    {
        const QStringList & paths = item->paths;

        for (int i = 0; i < paths.count(); i++)
        {
            qDebug("TORRENT RENAME ALL %d %s", i, paths.at(i).C_STR);

            handle.rename_file(i, paths.at(i).toStdString());
        }
    }
    else
    {
        qDebug("TORRENT RENAME FILE %d %s", item->index, fileName.C_STR);

        handle.rename_file(item->index, fileName.toStdString());
    }
}

//-------------------------------------------------------------------------------------------------

QString WTorrentEnginePrivate::extractFileName(const std::string & path, int index) const
{
    QString fileName = QString::fromStdString(path);

    QString extension = WControllerNetwork::extractUrlExtension(fileName);

    if (extension.isEmpty())
    {
         return QString::number(index + 1);
    }
    else return QString::number(index + 1) + "." + extension;
}

//-------------------------------------------------------------------------------------------------
// Cache

void WTorrentEnginePrivate::updateCache(WTorrentData * data)
{
    WTorrentSource * source = data->source;

    qint64 sourceSize = source->size;

    torrent_status status = data->handle.status(torrent_handle::query_accurate_download_counters);

    qint64 size = status.total_done;

    if (sourceSize == size) return;

    this->size -= sourceSize;

    if (size < _sizeMax)
    {
        qDebug("TORRENT RECACHING SOURCE");

        source->size = size;

        this->size += size;

        cleanCache();
    }
    else
    {
        Q_Q(WTorrentEngine);

        qWarning("WTorrentEnginePrivate::updateCache: File is too large for cache %s.",
                 source->urls.first().C_URL);

        sources.removeOne(source);

        deleteIds  .append(source->id);
        deletePaths.append(data->path);

        delete source;

        QTimer::singleShot(TORRENTENGINE_INTERVAL, q, SLOT(onFolderDelete()));

        save();
    }
}

//-------------------------------------------------------------------------------------------------

void WTorrentEnginePrivate::cleanCache()
{
    int index = 0;

    while (index < sources.count() && size >= _sizeMax)
    {
        if (removeSource(sources.at(index)) == false)
        {
            index++;
        }
    }

    if (index != sources.count()) save();
}

//-------------------------------------------------------------------------------------------------

bool WTorrentEnginePrivate::removeSource(WTorrentSource * source)
{
    qDebug("TORRENT REMOVE SOURCE");

    int id = source->id;

    QHashIterator<unsigned int, WTorrentData *> i(torrents);

    while (i.hasNext())
    {
        i.next();

        WTorrentData * data = i.value();

        if (data->source->id == id) return false;
    }

    Q_Q(WTorrentEngine);

    qDebug("TORRENT REMOVING SOURCE %d", id);

    sources.removeOne(source);

    size -= source->size;

    deleteIds  .append(id);
    deletePaths.append(path + QString::number(id));

    delete source;

    QTimer::singleShot(TORRENTENGINE_INTERVAL, q, SLOT(onFolderDelete()));

    return true;
}

//-------------------------------------------------------------------------------------------------

void WTorrentEnginePrivate::prioritize(const torrent_handle & handle,
                                       WTorrentStream       * stream, qint64 position) const
{
    int begin = stream->begin;
    int end   = stream->end;

    int current = begin + position / (qint64) stream->sizePiece;

    if (current >= end) return;

    //---------------------------------------------------------------------------------------------
    // Pieces

    WTorrentData * data = stream->data;

    QBitArray * pieces = &(data->pieces);

    if (stream->current != current)
    {
        while (current < end && pieces->at(current))
        {
            current++;
        }

        stream->piece   = current - begin;
        stream->current = current;
    }

    //---------------------------------------------------------------------------------------------
    // Blocks

    if (current == end)
    {
        stream->block = 0;

        stream->bufferPieces = stream->size;
        stream->bufferBlocks = stream->size;
    }
    else
    {
        int block;

        qint64 sizePiece = (qint64) stream->piece * (qint64) stream->sizePiece;

        if (position > sizePiece)
        {
             block = (position - sizePiece) / (qint64) TORRENTENGINE_BLOCK;
        }
        else block = 0;

        QBitArray * blocks = &(data->blocks);

        int blockCount = data->blockCount;

        int blockCurrent = current * blockCount + block;

        while (block < blockCount && blocks->at(blockCurrent))
        {
            block       ++;
            blockCurrent++;
        }

        stream->block = block;

        qint64 size = stream->size;

        qint64 bufferPieces = sizePiece - stream->start;

        qint64 bufferBlocks = bufferPieces + (qint64) block * (qint64) TORRENTENGINE_BLOCK;

        if (bufferPieces < size)
        {
            if (bufferPieces > 0)
            {
                 stream->bufferPieces = bufferPieces;
            }
            else stream->bufferPieces = 0;
        }
        else stream->bufferPieces = size;

        if (bufferBlocks < size)
        {
            if (bufferBlocks > 0)
            {
                 stream->bufferBlocks = bufferBlocks;
            }
            else stream->bufferBlocks = 0;
        }
        else stream->bufferBlocks = size;
    }

    //---------------------------------------------------------------------------------------------
    // Buffer

    QCoreApplication::postEvent(stream->torrent, new WTorrentEventBuffer(stream->bufferPieces,
                                                                         stream->bufferBlocks));

    //---------------------------------------------------------------------------------------------
    // Deadline

    bool clear = true;

    foreach (WTorrentItem * item, data->items)
    {
        if (stream != item && item->mode == WTorrent::Stream)
        {
            clear = false;
        }
    }

    // FIXME: Workaround to clear piece deadlines.
    if (clear) handle.clear_piece_deadlines();

    int priority = TORRENTENGINE_PRIORITY_COUNT;
    int deadline = TORRENTENGINE_PRIORITY_INTERVAL;

    while (priority && current < end)
    {
        if (pieces->at(current) == false)
        {
            handle.set_piece_deadline(current, deadline);

            deadline += TORRENTENGINE_PRIORITY_INTERVAL;

            priority--;
        }

        current++;
    }
}

//-------------------------------------------------------------------------------------------------

void WTorrentEnginePrivate::applyBlock(WTorrentStream * stream, int block)
{
    WTorrentData * data = stream->data;

    QBitArray * blocks = &(data->blocks);

    int blockCount = data->blockCount;

    int blockCurrent = stream->current * blockCount + block;

    block       ++;
    blockCurrent++;

    while (block < blockCount && blocks->at(blockCurrent))
    {
        block       ++;
        blockCurrent++;
    }

    qDebug("BLOCK COMPLETE %d %d", stream->piece, block);

    if (block < blockCount)
    {
        stream->block = block;

        applyBuffer(stream);
    }
}

void WTorrentEnginePrivate::applyPiece(const torrent_handle & handle,
                                       WTorrentStream       * stream, int current)
{
    qDebug("APPLY PIECE %d", current);

    WTorrentData * data = stream->data;

    //---------------------------------------------------------------------------------------------
    // Pieces

    QBitArray * pieces = &(data->pieces);

    int begin = stream->begin;
    int end   = stream->end;

    if (current == end)
    {
        for (int i = begin; i < end; i++)
        {
            if (pieces->at(i) == false) return;
        }

        qDebug("TORRENT STREAM FINISHED A");

        applyFinish(stream);

        return;
    }

    if (stream->current != current) return;

    current++;

    while (current < end && pieces->at(current))
    {
        current++;
    }

    stream->piece   = current - begin;
    stream->current = current;

    if (current == end)
    {
        qDebug("FILE AT END");

        qint64 size = stream->size;

        stream->bufferPieces = size;
        stream->bufferBlocks = size;

        QCoreApplication::postEvent(stream->torrent, new WTorrentEventBuffer(size, size));

        end--;

        for (int i = begin; i < end; i++)
        {
            if (pieces->at(i) == false) return;
        }

        qDebug("TORRENT STREAM FINISHED B");

        applyFinish(stream);

        return;
    }

    //---------------------------------------------------------------------------------------------
    // Blocks

    QBitArray * blocks = &(data->blocks);

    int block      = 0;
    int blockCount = data->blockCount;

    int blockCurrent = current * blockCount;

    while (block < blockCount && blocks->at(blockCurrent))
    {
        block       ++;
        blockCurrent++;
    }

    stream->block = block;

    //---------------------------------------------------------------------------------------------
    // Buffer

    applyBuffer(stream);

    //---------------------------------------------------------------------------------------------
    // Deadline

    int priority = TORRENTENGINE_PRIORITY_COUNT;
    int deadline = TORRENTENGINE_PRIORITY_INTERVAL;

    while (priority && current < end)
    {
        if (pieces->at(current) == false)
        {
            handle.set_piece_deadline(current, deadline);

            deadline += TORRENTENGINE_PRIORITY_INTERVAL;

            priority--;
        }

        current++;
    }
}

//-------------------------------------------------------------------------------------------------

void WTorrentEnginePrivate::applyBuffer(WTorrentStream * stream) const
{
    qint64 size = stream->size;

    qint64 bufferPieces = (qint64) stream->piece * (qint64) stream->sizePiece - stream->start;

    qint64 bufferBlocks = bufferPieces + (qint64) stream->block * (qint64) TORRENTENGINE_BLOCK;

    if (bufferPieces > size)
    {
        bufferPieces = size;
        bufferBlocks = size;
    }
    else if (bufferBlocks > size)
    {
        bufferPieces = bufferPieces;
        bufferBlocks = size;
    }

    if (stream->bufferPieces < bufferPieces)
    {
        stream->bufferPieces = bufferPieces;
        stream->bufferBlocks = bufferBlocks;

        QCoreApplication::postEvent(stream->torrent, new WTorrentEventBuffer(bufferPieces,
                                                                             bufferBlocks));
    }
    else if (stream->bufferBlocks < bufferBlocks)
    {
        stream->bufferBlocks = bufferBlocks;

        QCoreApplication::postEvent(stream->torrent, new WTorrentEventBuffer(stream->bufferPieces,
                                                                             bufferBlocks));
    }
}

void WTorrentEnginePrivate::applyFinish(WTorrentItem * item) const
{
    item->finished = true;

    QCoreApplication::postEvent(item->torrent, new WTorrentEvent(WTorrent::EventFinished));
}

//-------------------------------------------------------------------------------------------------

void WTorrentEnginePrivate::applyMagnet(WMagnetData * data, const torrent_handle & handle)
{
    const torrent_info & info = *(handle.torrent_file().get());

    create_torrent torrent(info);

    std::vector<char> vector;

    bencode(back_inserter(vector), torrent.generate());

    QByteArray bytes(vector.data(), vector.size());

    foreach (WMagnet * magnet, data->magnets)
    {
        QCoreApplication::postEvent(magnet, new WTorrentEventMagnet(bytes));
    }

    removeMagnet(data, handle);
}

void WTorrentEnginePrivate::removeMagnet(WMagnetData * data, const torrent_handle & handle) const
{
    QHashIterator<unsigned int, WTorrentData *> i(torrents);

    while (i.hasNext())
    {
        i.next();

        if (i.value()->hash == data->hash)
        {
            delete data;

            return;
        }
    }

    qDebug("MAGNET REMOVE HANDLE");

    session->remove_torrent(handle);

    WControllerFile::deleteFolder(pathMagnets);

    delete data;
}

//-------------------------------------------------------------------------------------------------

WTorrentData * WTorrentEnginePrivate::getData(const QUrl & url) const
{
    foreach (WTorrentData * data, datas)
    {
        foreach (const QUrl & source, data->source->urls)
        {
            if (source == url)
            {
                return data;
            }
        }
    }

    QHashIterator<unsigned int, WTorrentData *> i(torrents);

    while (i.hasNext())
    {
        i.next();

        WTorrentData * data = i.value();

        foreach (const QUrl & source, data->source->urls)
        {
            if (source == url)
            {
                return data;
            }
        }
    }

    return NULL;
}

WTorrentData * WTorrentEnginePrivate::getData(const sha1_hash & hash) const
{
    foreach (WTorrentData * data, datas)
    {
        if (data->source->hash == hash)
        {
            return data;
        }
    }

    return NULL;
}

//-------------------------------------------------------------------------------------------------

WMagnetData * WTorrentEnginePrivate::getMagnetData(const QUrl & url) const
{
    foreach (WMagnetData * data, datasMagnets)
    {
        if (data->url == url)
        {
            return data;
        }
    }

    QHashIterator<unsigned int, WMagnetData *> i(magnets);

    while (i.hasNext())
    {
        i.next();

        WMagnetData * data = i.value();

        if (data->url == url)
        {
            return data;
        }
    }

    return NULL;
}

WMagnetData * WTorrentEnginePrivate::getMagnetData(WMagnet * magnet) const
{
    foreach (WMagnetData * data, datasMagnets)
    {
        if (data->magnets.contains(magnet))
        {
            return data;
        }
    }

    QHashIterator<unsigned int, WMagnetData *> i(magnets);

    while (i.hasNext())
    {
        i.next();

        WMagnetData * data = i.value();

        if (data->magnets.contains(magnet))
        {
            return data;
        }
    }

    return NULL;
}

//-------------------------------------------------------------------------------------------------

WTorrentSource * WTorrentEnginePrivate::getSource(const sha1_hash & hash)
{
    foreach (WTorrentSource * source, sources)
    {
        if (source->hash == hash)
        {
            qDebug("TORRENT CACHED");

            sources.removeOne(source);
            sources.   append(source);

            return source;
        }
    }

    return NULL;
}

//-------------------------------------------------------------------------------------------------

WTorrentItem * WTorrentEnginePrivate::getItem(WTorrent * torrent) const
{
    foreach (WTorrentData * data, datas)
    {
        foreach (WTorrentItem * item, data->items)
        {
            if (item->torrent == torrent)
            {
                return item;
            }
        }
    }

    QHashIterator<unsigned int, WTorrentData *> i(torrents);

    while (i.hasNext())
    {
        i.next();

        foreach (WTorrentItem * item, i.value()->items)
        {
            if (item->torrent == torrent)
            {
                return item;
            }
        }
    }

    return NULL;
}

WTorrentStream * WTorrentEnginePrivate::getStream(WTorrent * torrent) const
{
    WTorrentItem * item = getItem(torrent);

    if (item && item->mode == WTorrent::Stream)
    {
         return static_cast<WTorrentStream *> (item);
    }
    else return NULL;
}

//-------------------------------------------------------------------------------------------------
// Private events
//-------------------------------------------------------------------------------------------------

void WTorrentEnginePrivate::events()
{
    std::vector<alert *> alerts;

    session->pop_alerts(&alerts);

    std::vector<alert *>::iterator i = alerts.begin();

    while (i != alerts.end())
    {
        alert * alert = *i;

        int type = alert->type();

        if (type == state_update_alert::alert_type)
        {
            state_update_alert * event = alert_cast<state_update_alert>(alert);

            std::vector<torrent_status> & status = event->status;

            if (status.empty()) return;

            Q_Q(WTorrentEngine);

            QList<WTorrentProgress> list;

            std::vector<torrent_status>::iterator i = status.begin();

            while (i != status.end())
            {
                const torrent_status & status = *i;

                WTorrentProgress progress;

                progress.hash = status.handle.id();

                progress.progress = status.total_done;

                progress.download = status.download_rate;
                progress.upload   = status.upload_rate;

                progress.seeds = status.num_seeds;
                progress.peers = status.num_peers;

                list.append(progress);

                i++;
            }

            QCoreApplication::postEvent(q, new WTorrentEngineProgress(list));
        }
        else if (type == torrent_added_alert::alert_type)
        {
            Q_Q(WTorrentEngine);

            torrent_added_alert * event = alert_cast<torrent_added_alert>(alert);

            QVariant variant;

            variant.setValue(event->handle);

            QCoreApplication::postEvent(q, new WTorrentEngineEvent(EventAdded, variant));
        }
        else if (type == metadata_received_alert::alert_type)
        {
            Q_Q(WTorrentEngine);

            metadata_received_alert * event = alert_cast<metadata_received_alert>(alert);

            QCoreApplication::postEvent(q, new WTorrentEngineHandle(EventMetaData,
                                                                    event->handle.id()));
        }
        else if (type == block_finished_alert::alert_type)
        {
            Q_Q(WTorrentEngine);

            block_finished_alert * event = alert_cast<block_finished_alert>(alert);

            QCoreApplication::postEvent(q, new WTorrentEngineBlock(event->handle.id(),
                                                                   event->piece_index,
                                                                   event->block_index));

        }
        else if (type == piece_finished_alert::alert_type)
        {
            Q_Q(WTorrentEngine);

            piece_finished_alert * event = alert_cast<piece_finished_alert>(alert);

            QCoreApplication::postEvent(q, new WTorrentEngineValue(EventPiece,
                                                                   event->handle.id(),
                                                                   event->piece_index));
        }
        else if (type == save_resume_data_alert::alert_type)
        {
            Q_Q(WTorrentEngine);

            save_resume_data_alert * event = alert_cast<save_resume_data_alert>(alert);

            unsigned int hash = event->handle.id();

            mutexA.lock();

            QString fileName = fileNames.take(hash);

            mutexA.unlock();

            std::ofstream stream(fileName.C_STR, std::ios_base::binary);

            stream.unsetf(std::ios_base::skipws);

            bencode(std::ostream_iterator<char>(stream), *(event->resume_data));

            QCoreApplication::postEvent(q, new WTorrentEngineHandle(EventSaved, hash));
        }
        else if (type == save_resume_data_failed_alert::alert_type)
        {
            Q_Q(WTorrentEngine);

            save_resume_data_failed_alert * event
                = alert_cast<save_resume_data_failed_alert>(alert);

            QCoreApplication::postEvent(q, new WTorrentEngineHandle(EventSaved,
                                                                    event->handle.id()));
        }
        else if (type == torrent_error_alert::alert_type)
        {
            Q_Q(WTorrentEngine);

            torrent_error_alert * event = alert_cast<torrent_error_alert>(alert);

            QString message = QString::fromStdString(event->message());

            QCoreApplication::postEvent(q, new WTorrentEngineValue(EventError,
                                                                   event->handle.id(), message));
        }

        i++;
    }
}

//-------------------------------------------------------------------------------------------------
// Private slots
//-------------------------------------------------------------------------------------------------

void WTorrentEnginePrivate::onUpdate()
{
    session->post_torrent_updates(torrent_handle::query_accurate_download_counters);
}

//-------------------------------------------------------------------------------------------------

void WTorrentEnginePrivate::onRemove()
{
    Q_Q(WTorrentEngine);

    qDebug("TORRENT ON REMOVE");

    QTimer * timer = static_cast<QTimer *> (q->sender());

    WTorrentData * data = deleteTorrents.take(timer);

    const torrent_handle & handle = data->handle;

    if (data->hash)
    {
        qDebug("REMOVE TORRENT");

        unsigned int hash = data->hash;

        torrents.remove(hash);

        if (torrents.isEmpty())
        {
            timerUpdate->stop();
        }

        if (magnets.contains(hash))
        {
            handle.move_storage(pathMagnets.toStdString());
        }
        else session->remove_torrent(handle);

        updateCache(data);
    }
    else
    {
        qDebug("REMOVE TORRENT ADD");

        datas.removeOne(data);

        if (magnets.contains(data->hash))
        {
            handle.move_storage(pathMagnets.toStdString());
        }
        else session->remove_torrent(handle);
    }

    delete data;

    timer->deleteLater();
}

void WTorrentEnginePrivate::onRemoveSource()
{
    WTorrentSource * source = deleteSources.takeFirst();

    if (sources.contains(source) && deleteIds.contains(source->id) == false)
    {
        if (removeSource(source))
        {
            if (sources.isEmpty())
            {
                Q_Q(WTorrentEngine);

                QTimer::singleShot(TORRENTENGINE_INTERVAL, q, SLOT(onFolderClear()));
            }
            else save();
        }
    }
}

//-------------------------------------------------------------------------------------------------

void WTorrentEnginePrivate::onFolderDelete()
{
    if (deleteIds.isEmpty()) return;

    int id = deleteIds.takeFirst();

    ids.removeOne(id);

    QString path = deletePaths.takeFirst();

    WControllerFile::deleteFolder(path);
}

void WTorrentEnginePrivate::onFolderClear()
{
    qDebug("TORRENT CACHE CLEARED");

    if (sources.isEmpty() == false) return;

    timerSave->stop();

    WControllerFile::deleteFolder(path);
}

//-------------------------------------------------------------------------------------------------

void WTorrentEnginePrivate::onSave()
{
    qDebug("TORRENT INDEX SAVED");

    if (QFile::exists(path) == false)
    {
        WControllerFile::createFolder(path);
    }

    QFile file(pathIndex);

    if (file.open(QIODevice::WriteOnly) == false)
    {
        qWarning("WTorrentEnginePrivate::onSave: Failed to open index %s.", pathIndex.C_STR);

        return;
    }

    QDataStream stream(&file);

    stream << sources.count();

    foreach (WTorrentSource * source, sources)
    {
        const sha1_hash & hash = source->hash;

        QString string = QString::fromLatin1(hash.data(), hash.size);

        const QList<QUrl> & urls = source->urls;

        stream << source->id << string << (qint64) source->size << urls.count();

        foreach (const QUrl & url, urls)
        {
            stream << url;
        }
    }
}

//-------------------------------------------------------------------------------------------------
// Ctor / dtor
//-------------------------------------------------------------------------------------------------

WTorrentEngine::WTorrentEngine(const QString & path, qint64 sizeMax, QThread * thread,
                                                                     QObject * parent)
    : QObject(parent), WPrivatable(new WTorrentEnginePrivate(this))
{
    Q_D(WTorrentEngine); d->init(path, sizeMax, thread);
}

//-------------------------------------------------------------------------------------------------
// Interface
//-------------------------------------------------------------------------------------------------

/* Q_INVOKABLE */ void WTorrentEngine::load(WTorrent * torrent, QIODevice * device)
{
    device->moveToThread(thread());

    QCoreApplication::postEvent(this, new WTorrentEngineAdd(torrent, device, torrent->url  (),
                                                                             torrent->index(),
                                                                             torrent->mode ()));
}

/* Q_INVOKABLE */ void WTorrentEngine::seek(WTorrent * torrent, qint64 position)
{
    if (position < 0) return;

    QCoreApplication::postEvent(this, new WTorrentEngineAction(WTorrentEnginePrivate::EventSeek,
                                                               torrent, position));
}

/* Q_INVOKABLE */ void WTorrentEngine::remove(WTorrent * torrent)
{
    QCoreApplication::postEvent(this, new WTorrentEngineItem(WTorrentEnginePrivate::EventRemove,
                                                             torrent));
}

//-------------------------------------------------------------------------------------------------

/* Q_INVOKABLE */ void WTorrentEngine::loadMagnet(WMagnet * magnet)
{
    QCoreApplication::postEvent(this,
                                new WTorrentEngineMagnet(WTorrentEnginePrivate::EventAddMagnet,
                                                         magnet));
}

/* Q_INVOKABLE */ void WTorrentEngine::removeMagnet(WMagnet * magnet)
{
    QCoreApplication::postEvent(this,
                                new WTorrentEngineMagnet(WTorrentEnginePrivate::EventRemoveMagnet,
                                                         magnet));
}

//-------------------------------------------------------------------------------------------------

/* Q_INVOKABLE */ void WTorrentEngine::clearSource(const QUrl & url)
{
    QCoreApplication::postEvent(this,
                                new WTorrentEngineEvent(WTorrentEnginePrivate::EventClearSource,
                                                        url));
}

/* Q_INVOKABLE */ void WTorrentEngine::clearCache()
{
    QCoreApplication::postEvent(this, new QEvent(static_cast<QEvent::Type>
                                                 (WTorrentEnginePrivate::EventClearCache)));
}

//-------------------------------------------------------------------------------------------------

/* Q_INVOKABLE */ void WTorrentEngine::deleteInstance()
{
    QCoreApplication::postEvent(this, new QEvent(static_cast<QEvent::Type>
                                                 (WTorrentEnginePrivate::EventClear)),
                                Qt::HighEventPriority * 100);
}

//-------------------------------------------------------------------------------------------------

/* Q_INVOKABLE */ void WTorrentEngine::setOptions(int connections, int upload, int download)
{
    Q_D(WTorrentEngine);

    d->mutexB.lock();

    d->connections = connections;

    d->upload   = upload;
    d->download = download;

    d->mutexB.unlock();

    QCoreApplication::postEvent(this, new QEvent(static_cast<QEvent::Type>
                                                 (WTorrentEnginePrivate::EventOptions)));
}

//-------------------------------------------------------------------------------------------------

/* Q_INVOKABLE */ void WTorrentEngine::setProxy(const QString & host,
                                                int             port, const QString & password)
{
    Q_D(WTorrentEngine);

    d->mutexB.lock();

    int index = host.indexOf('@');

    if (index == -1)
    {
        d->proxyHost = host;
        d->proxyUser = QString();
    }
    else
    {
        d->proxyHost = host.mid(index + 1);
        d->proxyUser = host.mid(0, index);
    }

    d->proxyPort     = port;
    d->proxyPassword = password;

    d->mutexB.unlock();

    QCoreApplication::postEvent(this, new QEvent(static_cast<QEvent::Type>
                                                 (WTorrentEnginePrivate::EventProxy)));
}

/* Q_INVOKABLE */ void WTorrentEngine::clearProxy()
{
    Q_D(WTorrentEngine);

    d->mutexB.lock();

    d->proxyHost = QString();
    d->proxyPort = 0;

    d->proxyUser     = QString();
    d->proxyPassword = QString();

    d->mutexB.unlock();

    QCoreApplication::postEvent(this, new QEvent(static_cast<QEvent::Type>
                                                 (WTorrentEnginePrivate::EventProxy)));
}

//-------------------------------------------------------------------------------------------------
// Events
//-------------------------------------------------------------------------------------------------

/* virtual */ bool WTorrentEngine::event(QEvent * event)
{
    Q_D(WTorrentEngine);

    QEvent::Type type = event->type();

    if (type == static_cast<QEvent::Type> (WTorrentEnginePrivate::EventCreate))
    {
        settings_pack pack;

        QString name = sk->name() + "/" + sk->version();

        pack.set_str(settings_pack::user_agent, name.toStdString());

        pack.set_int(settings_pack::alert_mask, alert::error_notification   |
                                                alert::storage_notification |
                                                alert::status_notification  |
                                                alert::progress_notification);

        pack.set_bool(settings_pack::enable_dht, true);
        pack.set_bool(settings_pack::enable_lsd, true);

        pack.set_bool(settings_pack::enable_upnp,   true);
        pack.set_bool(settings_pack::enable_natpmp, true);

        pack.set_bool(settings_pack::smooth_connects, false);

        pack.set_int(settings_pack::connection_speed, 500);

        //pack.set_int(settings_pack::max_failcount,      1);
        pack.set_int(settings_pack::min_reconnect_time, 1);

        pack.set_int(settings_pack::peer_connect_timeout, 3);
        pack.set_int(settings_pack::peer_timeout,         3);

        pack.set_int(settings_pack::piece_timeout,   3);
        pack.set_int(settings_pack::request_timeout, 3);

        //-----------------------------------------------------------------------------------------
        // FIXME: Workaround to improve writing efficiency.

        /*pack.set_int(settings_pack::use_disk_cache_pool, false);

        pack.set_int(settings_pack::cache_size,   0);
        pack.set_int(settings_pack::cache_expiry, 0);

        pack.set_int(settings_pack::disk_io_write_mode, settings_pack::disable_os_cache);*/

        //-----------------------------------------------------------------------------------------

        pack.set_bool(settings_pack::announce_to_all_tiers,    true);
        pack.set_bool(settings_pack::announce_to_all_trackers, true);

        //pack.set_bool(settings_pack::prioritize_partial_pieces, true);

        pack.set_int(settings_pack::stop_tracker_timeout, 1);

#ifdef LIBTORRENT_LATEST
        pack.set_str(settings_pack::dht_bootstrap_nodes, "router.bittorrent.com:6881,"
                                                         "router.utorrent.com:6881,"
                                                         "router.bitcomet.com:6881,"
                                                         "dht.libtorrent.org:25401,"
                                                         "dht.transmissionbt.com:6881,"
                                                         "dht.aelitis.com:6881");
#endif

        d->session = new session(pack);

        dht_settings dht;

        //dht.search_branching = 1;

        dht.max_fail_count = 3;

        //dht.max_dht_items =  1000;
        //dht.max_peers     = 10000;

        //d->session->set_dht_settings(dht);

#ifndef LIBTORRENT_LATEST
        d->session->add_dht_router(std::make_pair(std::string("router.bittorrent.com"),   6881));
        d->session->add_dht_router(std::make_pair(std::string("router.utorrent.com"),     6881));
        d->session->add_dht_router(std::make_pair(std::string("router.bitcomet.com"),     6881));
        d->session->add_dht_router(std::make_pair(std::string("dht.libtorrent.org"),     25401));
        d->session->add_dht_router(std::make_pair(std::string("dht.transmissionbt.com"),  6881));
        d->session->add_dht_router(std::make_pair(std::string("dht.aelitis.com"),         6881));
#endif

        boost::function<void()> alert(boost::bind(&WTorrentEnginePrivate::events, d));

        d->session->set_alert_notify(alert);

        d->session->add_extension(&create_ut_pex_plugin);

        d->pathIndex   = d->path + "index";
        d->pathMagnets = d->path + "magnets";

        d->size = 0;

        d->timerUpdate = new QTimer(this);

        d->timerUpdate->setInterval(TORRENTENGINE_INTERVAL);

        d->timerSave = new QTimer(this);

        d->timerSave->setInterval(TORRENTENGINE_INTERVAL);

        d->timerSave->setSingleShot(true);

        connect(d->timerUpdate, SIGNAL(timeout()), this, SLOT(onUpdate()));
        connect(d->timerSave,   SIGNAL(timeout()), this, SLOT(onSave  ()));

        d->load();

        return true;
    }
    else if (d->session == NULL)
    {
        return QObject::event(event);
    }
    else if (type == static_cast<QEvent::Type> (WTorrentEnginePrivate::EventAdd))
    {
        qDebug("TORRENT ADD");

        WTorrentEngineAdd * eventTorrent = static_cast<WTorrentEngineAdd *> (event);

        WTorrent * torrent = eventTorrent->torrent;

        QIODevice * device = eventTorrent->device;

        QUrl url   = eventTorrent->url;
        int  index = eventTorrent->index;

        WTorrent::Mode mode = static_cast<WTorrent::Mode> (eventTorrent->mode);

        WTorrentData * data = d->getData(url);

        if (mode == WTorrent::Stream)
        {
            if (data == NULL)
            {
                torrent_info * file = d->loadInfo(device->readAll());

                if (file == NULL)
                {
                    QCoreApplication::postEvent(torrent,
                                                new WTorrentEventValue(WTorrent::EventError,
                                                                       "Failed to load torrent."));

                    delete device;

                    return true;
                }

                boost::shared_ptr<torrent_info> info = boost::shared_ptr<torrent_info> (file);

                const sha1_hash & hash = info->info_hash();

                data = d->getData(hash);

                if (data)
                {
                    qDebug("TORRENT HASH EXISTS");

                    data->source->urls.append(url);

                    d->save();
                }
                else
                {
                    data = d->createData(info, hash, url);

                    d->createStream(info, data, torrent, index, mode);

                    delete device;

                    return true;
                }
            }

            d->updateData(data);

            const torrent_handle & handle = data->handle;

            WTorrentStream * stream = d->createStream(handle.torrent_file(),
                                                      data, torrent, index, mode);

            if (data->hash)
            {
                d->renameFile(handle, stream);

                d->addStream(handle, stream);

                d->selectFile(stream);
            }
        }
        else
        {
            if (data == NULL)
            {
                torrent_info * file = d->loadInfo(device->readAll());

                if (file == NULL)
                {
                    QCoreApplication::postEvent(torrent,
                                                new WTorrentEventValue(WTorrent::EventError,
                                                                       "Failed to load torrent."));

                    delete device;

                    return true;
                }

                boost::shared_ptr<torrent_info> info = boost::shared_ptr<torrent_info> (file);

                const sha1_hash & hash = info->info_hash();

                data = d->getData(hash);

                if (data)
                {
                    qDebug("TORRENT HASH EXISTS");

                    data->source->urls.append(url);

                    d->save();
                }
                else
                {
                    data = d->createData(info, hash, url);

                    d->createItem(info, data, torrent, index, mode);

                    delete device;

                    return true;
                }
            }

            d->updateData(data);

            const torrent_handle & handle = data->handle;

            WTorrentItem * item = d->createItem(handle.torrent_file(), data, torrent, index, mode);

            if (data->hash)
            {
                d->renameFile(handle, item);

                d->addItem(handle, item);

                d->selectFile(item);
            }
        }

        delete device;

        return true;
    }
    else if (type == static_cast<QEvent::Type> (WTorrentEnginePrivate::EventAddMagnet))
    {
        qDebug("TORRENT ADD MAGNET");

        WTorrentEngineMagnet * eventTorrent = static_cast<WTorrentEngineMagnet *> (event);

        WMagnet * magnet = eventTorrent->magnet;

        QUrl url = magnet->url();

        WMagnetData * data = d->getMagnetData(url);

        if (data == NULL)
        {
            qDebug("MAGNET CREATE DATA");

            add_torrent_params params;

            params.url = magnet->url().toString().toStdString();

            params.save_path = d->pathMagnets.toStdString();

            data = new WMagnetData;

            data->url  = url;
            data->hash = 0;

            data->magnets.append(magnet);

            d->datas       .append(NULL);
            d->datasMagnets.append(data);

            d->session->async_add_torrent(params);
        }
        else if (data->magnets.contains(magnet) == false)
        {
            qDebug("MAGNET ALREADY EXISTS");

            data->magnets.append(magnet);
        }

        return true;
    }
    else if (type == static_cast<QEvent::Type> (WTorrentEnginePrivate::EventAdded))
    {
        WTorrentEngineEvent * eventTorrent = static_cast<WTorrentEngineEvent *> (event);

        WTorrentData * data = d->datas.takeFirst();

        if (data == NULL)
        {
            qDebug("MAGNET ADDED");

            WMagnetData * data = d->datasMagnets.takeFirst();

            const torrent_handle & handle = eventTorrent->value.value<torrent_handle>();

            if (data->magnets.isEmpty())
            {
                d->removeMagnet(data, handle);

                return true;
            }

            unsigned int hash = handle.id();

            data->handle = handle;
            data->hash   = hash;

            if (handle.status().has_metadata)
            {
                qDebug("METADATA ALREADY DONE");

                d->applyMagnet(data, handle);
            }
            else
            {
                qDebug("METADATA STARTED");

                d->magnets.insert(hash, data);
            }

            return true;
        }

        qDebug("TORRENT ADDED");

        if (data->items.isEmpty())
        {
            d->removeData(data);

            return true;
        }

        const torrent_handle & handle = eventTorrent->value.value<torrent_handle>();

        unsigned int hash = handle.id();

        WMagnetData * dataMagnet = d->magnets.value(hash);

        if (dataMagnet)
        {
            qDebug("TORRENT MOVE STORAGE");

            handle.move_storage(data->path.toStdString());
        }

        data->handle = handle;
        data->hash   = hash;

        d->torrents.insert(hash, data);

        d->renameFiles(data, handle);

        foreach (WTorrentItem * item, data->items)
        {
            if (item->mode == WTorrent::Stream)
            {
                WTorrentStream * stream = static_cast<WTorrentStream *> (item);

                d->addStream(handle, stream);
            }
            else d->addItem(handle, item);
        }

        d->updateFiles(data);

        d->timerUpdate->start();

        return true;
    }
    else if (type == static_cast<QEvent::Type> (WTorrentEnginePrivate::EventMetaData))
    {
        qDebug("TORRENT METADATA");

        WTorrentEngineHandle * eventTorrent = static_cast<WTorrentEngineHandle *> (event);

        WTorrentData * data = d->torrents.value(eventTorrent->hash);

        if (data) d->renameFiles(data, data->handle);

        WMagnetData * dataMagnet = d->magnets.value(eventTorrent->hash);

        if (dataMagnet == NULL) return true;

        d->magnets.remove(dataMagnet->hash);

        d->applyMagnet(dataMagnet, dataMagnet->handle);

        return true;
    }
    else if (type == static_cast<QEvent::Type> (WTorrentEnginePrivate::EventSeek))
    {
        WTorrentEngineAction * eventTorrent = static_cast<WTorrentEngineAction *> (event);

        WTorrentStream * stream = d->getStream(eventTorrent->torrent);

        if (stream->finished)
        {
            qint64 size = stream->size;

            QCoreApplication::postEvent(stream->torrent, new WTorrentEventBuffer(size, size));
        }
        else
        {
            WTorrentData * data = stream->data;

            if (data->hash)
            {
                d->prioritize(data->handle, stream, eventTorrent->value.toLongLong());
            }
            else stream->position = eventTorrent->value.toLongLong();
        }

        return true;
    }
    else if (type == static_cast<QEvent::Type> (WTorrentEnginePrivate::EventRemove))
    {
        WTorrentEngineItem * eventTorrent = static_cast<WTorrentEngineItem *> (event);

        WTorrentItem * item = d->getItem(eventTorrent->torrent);

        if (item == NULL) return true;

        WTorrentData * data = item->data;

        QList<WTorrentItem *> * items = &(data->items);

        items->removeOne(item);

        if (items->isEmpty() == false)
        {
            d->unselectFile(item);

            delete item;

            return true;
        }

        delete item;

        if (data->hash == 0) return true;

        int id = data->source->id;

        QString fileName = data->path + "/." + QString::number(id);

        d->mutexA.lock();

        d->fileNames.insert(data->hash, fileName);

        d->mutexA.unlock();

        data->handle.save_resume_data();

        return true;
    }
    else if (type == static_cast<QEvent::Type> (WTorrentEnginePrivate::EventRemoveMagnet))
    {
        qDebug("MAGNET REMOVE");

        WTorrentEngineMagnet * eventTorrent = static_cast<WTorrentEngineMagnet *> (event);

        WMagnet * magnet = eventTorrent->magnet;

        WMagnetData * data = d->getMagnetData(magnet);

        if (data == NULL) return true;

        QList<WMagnet *> * magnets = &(data->magnets);

        magnets->removeOne(magnet);

        if (magnets->isEmpty() == false) return true;

        unsigned int hash = data->hash;

        if (hash == 0) return true;

        d->magnets.remove(hash);

        d->removeMagnet(data, data->handle);

        return true;
    }
    else if (type == static_cast<QEvent::Type> (WTorrentEnginePrivate::EventSaved))
    {
        WTorrentEngineHandle * eventTorrent = static_cast<WTorrentEngineHandle *> (event);

        WTorrentData * data = d->torrents.value(eventTorrent->hash);

        if (data == NULL)
        {
            qDebug("EventSaved: DATA SHOULD NOT BE NULL");
        }

        qDebug("TORRENT SAVED");

        if (data->items.isEmpty())
        {
            d->removeData(data);
        }

        return true;
    }
    else if (type == static_cast<QEvent::Type> (WTorrentEnginePrivate::EventProgress))
    {
        WTorrentEngineProgress * eventProgress = static_cast<WTorrentEngineProgress *> (event);

        const QList<WTorrentProgress> & list = eventProgress->list;

        foreach (const WTorrentProgress & value, list)
        {
            WTorrentData * data = d->torrents.value(value.hash);

            if (data == NULL) continue;

            foreach (WTorrentItem * item, data->items)
            {
                QCoreApplication::postEvent(item->torrent,
                                            new WTorrentEventProgress(value.progress,
                                                                      value.download, value.upload,
                                                                      value.seeds, value.peers));
            }
        }

        return true;
    }
    else if (type == static_cast<QEvent::Type> (WTorrentEnginePrivate::EventBlock))
    {
        WTorrentEngineBlock * eventTorrent = static_cast<WTorrentEngineBlock *> (event);

        WTorrentData * data = d->torrents.value(eventTorrent->hash);

        if (data == NULL) return true;

        int piece = eventTorrent->piece;
        int block = eventTorrent->block;

        data->blocks.setBit(piece * data->blockCount + block);

        foreach (WTorrentItem * item, data->items)
        {
            if (item->mode == WTorrent::Stream)
            {
                WTorrentStream * stream = static_cast<WTorrentStream *> (item);

                if (stream->finished == false && stream->current == piece && stream->block == block)
                {
                    d->applyBlock(stream, block);
                }
            }
        }

        return true;
    }
    else if (type == static_cast<QEvent::Type> (WTorrentEnginePrivate::EventPiece))
    {
        WTorrentEngineValue * eventTorrent = static_cast<WTorrentEngineValue *> (event);

        WTorrentData * data = d->torrents.value(eventTorrent->hash);

        if (data == NULL) return true;

        int piece = eventTorrent->value.toInt();

        data->pieces.setBit(piece);

        int block      = 0;
        int blockCount = data->blockCount;

        int blockCurrent = piece * blockCount;

        while (block < blockCount)
        {
            data->blocks.setBit(blockCurrent);

            block       ++;
            blockCurrent++;
        }

        foreach (WTorrentItem * item, data->items)
        {
            if (item->mode == WTorrent::Stream)
            {
                WTorrentStream * stream = static_cast<WTorrentStream *> (item);

                if (stream->finished == false)
                {
                    d->applyPiece(data->handle, stream, piece);
                }
            }
            else if (item->finished == false)
            {
                int current = item->current;

                if (current != piece) return true;

                QBitArray * pieces = &(data->pieces);

                int end = item->end;

                current++;

                while (current < end && pieces->at(current))
                {
                    current++;
                }

                item->current = current;

                if (current == end)
                {
                    d->applyFinish(item);
                }
            }
        }

        return true;
    }
    else if (type == static_cast<QEvent::Type> (WTorrentEnginePrivate::EventError))
    {
        WTorrentEngineValue * eventTorrent = static_cast<WTorrentEngineValue *> (event);

        unsigned int hash = eventTorrent->hash;

        QVariant value = eventTorrent->value;

        WTorrentData * data = d->torrents.value(hash);

        if (data)
        {
            qDebug("TORRENT ERROR");

            foreach (WTorrentItem * item, data->items)
            {
                QCoreApplication::postEvent(item->torrent,
                                            new WTorrentEventValue(WTorrent::EventError, value));
            }
        }

        WMagnetData * dataMagnet = d->magnets.value(eventTorrent->hash);

        if (dataMagnet)
        {
            qDebug("MAGNET ERROR");

            foreach (WMagnet * magnet, dataMagnet->magnets)
            {
                QCoreApplication::postEvent(magnet,
                                            new WTorrentEventValue(WTorrent::EventError, value));
            }

            d->magnets.remove(dataMagnet->hash);

            d->removeMagnet(dataMagnet, dataMagnet->handle);
        }

        return true;
    }
    else if (type == static_cast<QEvent::Type> (WTorrentEnginePrivate::EventOptions))
    {
        qDebug("TORRENT OPTIONS");

        settings_pack pack = d->session->get_settings();

        d->mutexB.lock();

        pack.set_int(settings_pack::connection_speed, d->connections);

        pack.set_int(settings_pack::upload_rate_limit,   d->upload);
        pack.set_int(settings_pack::download_rate_limit, d->download);

        d->mutexB.unlock();

        d->session->apply_settings(pack);

        return true;
    }
    else if (type == static_cast<QEvent::Type> (WTorrentEnginePrivate::EventProxy))
    {
        qDebug("TORRENT PROXY");

        settings_pack pack = d->session->get_settings();

        d->mutexB.lock();

        pack.set_str(settings_pack::proxy_hostname, d->proxyHost.C_STR);
        pack.set_int(settings_pack::proxy_port,     d->proxyPort);

        pack.set_str(settings_pack::proxy_username, d->proxyUser    .C_STR);
        pack.set_str(settings_pack::proxy_password, d->proxyPassword.C_STR);

        d->mutexB.unlock();

        d->session->apply_settings(pack);

        return true;
    }
    else if (type == static_cast<QEvent::Type> (WTorrentEnginePrivate::EventSizeMax))
    {
        WTorrentEngineEvent * eventTorrent = static_cast<WTorrentEngineEvent *> (event);

        d->_sizeMax = eventTorrent->value.toLongLong();

        d->cleanCache();

        return true;
    }
    else if (type == static_cast<QEvent::Type> (WTorrentEnginePrivate::EventClearSource))
    {
        WTorrentEngineEvent * eventTorrent = static_cast<WTorrentEngineEvent *> (event);

        QUrl url = eventTorrent->value.toUrl();

        foreach (WTorrentSource * source, d->sources)
        {
            foreach (const QUrl & sourceUrl, source->urls)
            {
                if (sourceUrl == url)
                {
                    qDebug("TORRENT CLEAR SOURCE");

                    d->deleteSources.append(source);

                    QTimer::singleShot(TORRENTENGINE_INTERVAL_REMOVE, this,
                                       SLOT(onRemoveSource()));
                }
            }
        }

        return true;
    }
    else if (type == static_cast<QEvent::Type> (WTorrentEnginePrivate::EventClearCache))
    {
        qDebug("TORRENT CLEAR CACHE");

        if (d->sources.isEmpty() == false)
        {
            foreach (WTorrentSource * source, d->sources)
            {
                d->deleteSources.append(source);

                QTimer::singleShot(TORRENTENGINE_INTERVAL_REMOVE, this, SLOT(onRemoveSource()));
            }
        }
        else QTimer::singleShot(TORRENTENGINE_INTERVAL, this, SLOT(onFolderClear()));

        return true;
    }
    else if (type == static_cast<QEvent::Type> (WTorrentEnginePrivate::EventClear))
    {
        d->timerUpdate->stop();

        qDebug("TORRENT SESSION BEFORE");

        delete d->session;

        qDebug("TORRENT SESSION AFTER");

        d->session = NULL;

        QHashIterator<QTimer *, WTorrentData *> i(d->deleteTorrents);

        while (i.hasNext())
        {
            i.next();

            delete i.key();
        }

        foreach (WTorrentData * data, d->datas)
        {
            delete data;
        }

        foreach (WMagnetData * data, d->datasMagnets)
        {
            delete data;
        }

        QHashIterator<unsigned int, WTorrentData *> j(d->torrents);

        while (j.hasNext())
        {
            j.next();

            delete j.value();
        }

        QHashIterator<unsigned int, WMagnetData *> k(d->magnets);

        while (k.hasNext())
        {
            k.next();

            delete k.value();
        }

        if (d->timerSave->isActive())
        {
            d->onSave();
        }

        foreach (WTorrentSource * source, d->sources)
        {
            delete source;
        }

        return true;
    }
    else return QObject::event(event);
}

//-------------------------------------------------------------------------------------------------
// Properties
//-------------------------------------------------------------------------------------------------

qint64 WTorrentEngine::sizeMax() const
{
    Q_D(const WTorrentEngine); return d->sizeMax;
}

void WTorrentEngine::setSizeMax(qint64 max)
{
    Q_D(WTorrentEngine);

    if (d->sizeMax == max) return;

    d->sizeMax = max;

    QCoreApplication::postEvent(this, new WTorrentEngineEvent(WTorrentEnginePrivate::EventSizeMax,
                                                              max));
}

#endif // SK_NO_TORRENTENGINE
