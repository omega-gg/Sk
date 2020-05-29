//=================================================================================================
/*
    Copyright (C) 2015-2020 Sky kit authors united with omega. <http://omega.gg/about>

    Author: Benjamin Arnaud. <http://bunjee.me> <bunjee@omega.gg>

    This file is part of Sky kit.

    - GNU Lesser General Public License Usage:
    This file may be used under the terms of the GNU Lesser General Public License version 3 as
    published by the Free Software Foundation and appearing in the LICENSE.md file included in the
    packaging of this file. Please review the following information to ensure the GNU Lesser
    General Public License requirements will be met: https://www.gnu.org/licenses/lgpl.html.
*/
//=================================================================================================

#ifndef WCONTROLLERFILE_H
#define WCONTROLLERFILE_H

// Qt includes
#include <QFileInfo>
#include <QUrl>

// Sk includes
#include <WController>
#include <WAbstractThreadAction>
#include <WAbstractThreadReply>

#ifndef SK_NO_CONTROLLERFILE

// Forward declarations
namespace QtLP_Private
{
    class QtLockedFile;
};
class WControllerFilePrivate;
class WControllerFileReply;
class WCache;
class WCacheFile;

// Defines
#define wControllerFile WControllerFile::instance()

//=================================================================================================
// WControllerFileAction
//=================================================================================================

class SK_CORE_EXPORT WControllerFileAction : public WAbstractThreadAction
{
    Q_OBJECT

protected: // WAbstractThreadAction reimplementation
    /* virtual */ WAbstractThreadReply * createReply() const;

public: // Properties
    WControllerFileReply * controllerReply();
};

//=================================================================================================
// WControllerFileReply
//=================================================================================================

class SK_CORE_EXPORT WControllerFileReply : public WAbstractThreadReply
{
    Q_OBJECT

protected:
    WControllerFileReply() {}

protected: // WAbstractThreadReply reimplementation
    /* virtual */ void onCompleted(bool ok);

signals:
    void actionComplete(bool ok);

private:
    Q_DISABLE_COPY(WControllerFileReply)

    friend class WControllerFileAction;
};

//=================================================================================================
// WControllerFile
//=================================================================================================

class SK_CORE_EXPORT WControllerFile : public WController
{
    Q_OBJECT

    Q_PROPERTY(QString log READ log NOTIFY logChanged)

    Q_PROPERTY(QString pathStorage READ pathStorage WRITE setPathStorage NOTIFY pathStorageChanged)

    Q_PROPERTY(QString pathLog READ pathLog NOTIFY pathStorageChanged)

    Q_PROPERTY(WCache * cache READ cache WRITE setCache NOTIFY cacheChanged)

private:
    WControllerFile();

protected: // Initialize
    /* virtual */ void init();

public: // Interface
    Q_INVOKABLE void initMessageHandler();

    Q_INVOKABLE WCacheFile * getFile(const QString & url,
                                     QObject       * parent = NULL, int maxHost = -1);

    Q_INVOKABLE WCacheFile * getHttp(const QString & url,
                                     QObject       * parent = NULL, int maxHost = -1);

    Q_INVOKABLE QString getFileUrl(const QString & url);

    Q_INVOKABLE WCacheFile * writeCache(const QString    & url,
                                        const QByteArray & array,
                                        const QString    & extension = QString(),
                                        QObject          * parent    = NULL);

    Q_INVOKABLE void addCache(const QString & url, const QByteArray & array,
                                                   const QString    & extension = QString());

    Q_INVOKABLE void waitActions();

    //---------------------------------------------------------------------------------------------
    // Thread actions

    WAbstractThreadReply * startWriteAction(WAbstractThreadAction * action);
    WAbstractThreadReply * startReadAction (WAbstractThreadAction * action);

    WControllerFileReply * startWriteFiles(const QStringList       & fileNames,
                                           const QList<QByteArray> & datas);

    WControllerFileReply * startAppendFiles(const QStringList       & fileNames,
                                            const QList<QByteArray> & datas);

    WControllerFileReply * startRenameFiles(const QStringList & oldPaths,
                                            const QStringList & newPaths);

    WControllerFileReply * startCopyFiles(const QStringList & fileNames,
                                          const QStringList & newNames);

    WControllerFileReply * startDeleteFiles(const QStringList & paths);

    WControllerFileReply * startCreateFolders(const QStringList & paths);

    WControllerFileReply * startDeleteFolders(const QStringList & paths,
                                              bool                recursive = true);

    WControllerFileReply * startDeleteFoldersContent(const QStringList & paths,
                                                     bool                recursive = true);

    WControllerFileReply * startCreatePaths(const QStringList & paths);

    //---------------------------------------------------------------------------------------------

    WControllerFileReply * startWriteFile(const QString & fileName, const QByteArray & data);

    WControllerFileReply * startAppendFile(const QString & fileName, const QByteArray & data);

    WControllerFileReply * startRenameFile(const QString & oldPath, const QString & newPath);

    WControllerFileReply * startCopyFile(const QString & fileName, const QString & newName);

    WControllerFileReply * startDeleteFile(const QString & path);

    WControllerFileReply * startCreateFolder       (const QString & path);
    WControllerFileReply * startDeleteFolder       (const QString & path, bool recursive = true);
    WControllerFileReply * startDeleteFolderContent(const QString & path, bool recursive = true);

    WControllerFileReply * startCreatePath(const QString & path);

public: // Static functions
    Q_INVOKABLE static QString absolute(const QUrl    & url);
    Q_INVOKABLE static QString absolute(const QString & string);

    Q_INVOKABLE static QString fileUrl(const QUrl    & url);
    Q_INVOKABLE static QString fileUrl(const QString & string);

    Q_INVOKABLE static QString filePath(const QUrl    & url);
    Q_INVOKABLE static QString filePath(const QString & string);

    Q_INVOKABLE static QString fileAbsolute(const QUrl    & url);
    Q_INVOKABLE static QString fileAbsolute(const QString & string);

    Q_INVOKABLE static QString currentPath   (const QString & fileName);
    Q_INVOKABLE static QString currentFileUrl(const QString & fileName);

    Q_INVOKABLE static QString applicationPath   (const QString & fileName);
    Q_INVOKABLE static QString applicationFileUrl(const QString & fileName);

    Q_INVOKABLE static QString folderPath(const QString & fileName);
    Q_INVOKABLE static QString folderUrl (const QString & fileName);

    Q_INVOKABLE static QString folderName(const QString & fileName);

    Q_INVOKABLE static QString toLocalFile(const QString & url);
    Q_INVOKABLE static QString toString   (const QString & url);

    //---------------------------------------------------------------------------------------------

    Q_INVOKABLE static QByteArray readAll(const QString & fileName);

    Q_INVOKABLE static bool exists   (const QString & fileName);
    Q_INVOKABLE static bool tryAppend(const QString & fileName);

    //---------------------------------------------------------------------------------------------
    // Files

    static bool tryUnlock(const QtLP_Private::QtLockedFile & file,
                          int                                timeout = 10000); // 10 seconds

    static bool writeFile(const QString & fileName, const QByteArray & data);

    static bool appendFile(const QString & fileName, const QByteArray & data);

    static bool renameFile(const QString & oldPath, const QString & newPath);

    static bool copyFile(const QString & fileName, const QString & newName);

    static bool deleteFile(const QString & fileName);

    //---------------------------------------------------------------------------------------------
    // Directories

    static bool createFolder(const QString & path);

    static bool moveFolder(const QString & oldPath, const QString & newPath);

    static bool deleteFolder       (const QString & path, bool recursive = true);
    static bool deleteFolderContent(const QString & path, bool recursive = true);

    static QFileInfoList recursiveEntryInfoList(const QString & path);

signals:
    void logChanged(const QString & message);

    void pathStorageChanged();

    void cacheChanged();

public: // Properties
    QString log() const;

    QString pathStorage() const;
    QString pathLog    () const;

    void setPathStorage(const QString & path);

    WCache * cache() const;
    void     setCache(WCache * cache);

private:
    W_DECLARE_PRIVATE   (WControllerFile)
    W_DECLARE_CONTROLLER(WControllerFile)

    Q_PRIVATE_SLOT(d_func(), void onLog(const QString &))

    Q_PRIVATE_SLOT(d_func(), void onWriteLog())

    Q_PRIVATE_SLOT(d_func(), void onCheckWatchers())

    friend class WFileWatcher;
    friend class WFileWatcherPrivate;
    friend class WLocalObject;
    friend class WLocalObjectPrivate;
};

#include <private/WControllerFile_p>

#endif // SK_NO_CONTROLLERFILE
#endif // WCONTROLLERFILE_H
