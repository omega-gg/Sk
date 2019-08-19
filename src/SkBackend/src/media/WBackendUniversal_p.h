//=================================================================================================
/*
    Copyright (C) 2015-2017 Sky kit authors united with omega. <http://omega.gg/about>

    Author: Benjamin Arnaud. <http://bunjee.me> <bunjee@omega.gg>

    This file is part of the SkBackend module of Sky kit.

    - GNU General Public License Usage:
    This file may be used under the terms of the GNU General Public License version 3 as published
    by the Free Software Foundation and appearing in the LICENSE.md file included in the packaging
    of this file. Please review the following information to ensure the GNU General Public License
    requirements will be met: https://www.gnu.org/licenses/gpl.html.
*/
//=================================================================================================

#ifndef WBACKENDUNIVERSAL_P_H
#define WBACKENDUNIVERSAL_P_H

/*  W A R N I N G
    -------------

    This file is not part of the Sk API. It exists purely as an
    implementation detail. This header file may change from version to
    version without notice, or even be removed.

    We mean it.
*/

// Private includes
#include <private/WBackendNet_p>

#ifndef SK_NO_BACKENDUNIVERSAL

// Forward declarations
class WRemoteData;
class WBackendUniversalData;
class WBackendUniversalParameters;

//=================================================================================================
// WBackendUniversalData
//=================================================================================================

struct WBackendUniversalData
{
    WBackendUniversalData()
    {
        //hasSearch = false;

        isSearchEngine = false;
        isSearchCover  = false;
    }

    QString api;

    //bool hasSearch;

    bool isSearchEngine;
    bool isSearchCover;

    QString title;
    QString host;

    QList<WLibraryFolderItem> items;

    //---------------------------------------------------------------------------------------------

    QString validate;

    QString trackId;
    QString trackOutput;

    QString playlistInfo;

    QString urlTrack;
    QString urlPlaylist;

    QString querySource;
    QString queryTrack;
    QString queryPlaylist;
    QString queryFolder;
    QString queryItem;

    QString createQuery;

    QString extractSource;
    QString extractTrack;
    QString extractPlaylist;
    QString extractFolder;
    QString extractItem;

    QString applySource;
    QString applyTrack;
    QString applyPlaylist;
    QString applyFolder;
    QString applyItem;
};

//=================================================================================================
// WBackendUniversalNode
//=================================================================================================

class SK_BACKEND_EXPORT WBackendUniversalNode
{
public: // Enums
    enum Type
    {
        String,
        Variable,
        Function
    };

public:
    WBackendUniversalNode(Type type);

public: // Interface
    QVariant run(WBackendUniversalParameters * parameters) const;

    void dump(int indent = 0) const;

    QVariant getVariant(WBackendUniversalParameters * parameters, int index) const;

    int   getInt  (WBackendUniversalParameters * parameters, int index) const;
    float getFloat(WBackendUniversalParameters * parameters, int index) const;

    QByteArray getByteArray(WBackendUniversalParameters * parameters, int index) const;
    QString    getString   (WBackendUniversalParameters * parameters, int index) const;

    QRegExp getRegExp(WBackendUniversalParameters * parameters, int index) const;

    QVariant       * getValue     (WBackendUniversalParameters * parameters, int index) const;
    const QVariant * getValueConst(WBackendUniversalParameters * parameters, int index) const;

private: // Functions
    inline QVariant equals   (WBackendUniversalParameters * parameters) const;
    inline QVariant notEquals(WBackendUniversalParameters * parameters) const;

    inline QVariant lower  (WBackendUniversalParameters * parameters) const;
    inline QVariant greater(WBackendUniversalParameters * parameters) const;

    inline QVariant number  (WBackendUniversalParameters * parameters) const;
    inline QVariant add     (WBackendUniversalParameters * parameters) const;
    inline QVariant sub     (WBackendUniversalParameters * parameters) const;
    inline QVariant multiply(WBackendUniversalParameters * parameters) const;

    inline QVariant set    (WBackendUniversalParameters * parameters) const;
    inline QVariant setHash(WBackendUniversalParameters * parameters) const;

    inline QVariant prepend   (WBackendUniversalParameters * parameters) const;
    inline QVariant append    (WBackendUniversalParameters * parameters) const;
    inline QVariant appendList(WBackendUniversalParameters * parameters) const;

    inline QVariant read(WBackendUniversalParameters * parameters) const;

    inline QVariant length(WBackendUniversalParameters * parameters) const;

    inline QVariant indexOf    (WBackendUniversalParameters * parameters) const;
    inline QVariant indexRegExp(WBackendUniversalParameters * parameters) const;

    inline QVariant indexEnd      (WBackendUniversalParameters * parameters) const;
    inline QVariant indexRegExpEnd(WBackendUniversalParameters * parameters) const;

    inline QVariant lastIndexOf    (WBackendUniversalParameters * parameters) const;
    inline QVariant lastIndexRegExp(WBackendUniversalParameters * parameters) const;

    inline QVariant lastIndexEnd      (WBackendUniversalParameters * parameters) const;
    inline QVariant lastIndexRegExpEnd(WBackendUniversalParameters * parameters) const;

    inline QVariant contains(WBackendUniversalParameters * parameters) const;

    inline QVariant startsWith      (WBackendUniversalParameters * parameters) const;
    inline QVariant startsWithRegExp(WBackendUniversalParameters * parameters) const;

    inline QVariant removeChars (WBackendUniversalParameters * parameters) const;
    inline QVariant removePrefix(WBackendUniversalParameters * parameters) const;

    inline QVariant addQuery(WBackendUniversalParameters * parameters) const;

    inline QVariant extractUrlElement(WBackendUniversalParameters * parameters) const;

    inline QVariant extractJson    (WBackendUniversalParameters * parameters) const;
    inline QVariant extractJsonUtf8(WBackendUniversalParameters * parameters) const;
    inline QVariant extractJsonHtml(WBackendUniversalParameters * parameters) const;

    inline QVariant splitJson(WBackendUniversalParameters * parameters) const;

    inline QVariant print(WBackendUniversalParameters * parameters) const;

public: // Variables
    Type type;

    QString data;

    QList<WBackendUniversalNode> nodes;
};

//=================================================================================================
// WBackendUniversalScript
//=================================================================================================

class SK_BACKEND_EXPORT WBackendUniversalScript
{
public:
    WBackendUniversalScript(const QString & data);

public: // Interface
    bool isValid() const;

    QVariant run(WBackendUniversalParameters * parameters) const;

    void dump() const;

private: // Functions
    void load(const QString & data);

    bool loadParameters(WBackendUniversalNode * node,
                        QString               * string, const QRegExp & regExp) const;

    bool loadFunction(WBackendUniversalNode * node,
                      QString               * string, const QRegExp & regExp) const;

    QString extractWord  (QString * string) const;
    QString extractString(QString * string) const;

    bool skipCondition(int * index) const;

    void skipLoop(int * index) const;

    void skipOperators(int * index) const;

    bool getCondition(WBackendUniversalParameters * parameters,
                      const WBackendUniversalNode & node, int * index) const;

public: // Properties
    QList<WBackendUniversalNode> nodes;
};

//=================================================================================================
// WBackendUniversalParameters
//=================================================================================================

class SK_BACKEND_EXPORT WBackendUniversalParameters
{
public:
    WBackendUniversalParameters(const WBackendUniversalScript & script, const QVariant & global);

public: // Interface
    void add(const QString & name, const QVariant & value = QVariant());

    QVariant       * value     (const QString & name);
    const QVariant * valueConst(const QString & name) const;

private: // Functions
    void extract(QStringList * list, const WBackendUniversalNode & node);

public: // Variables
    QHash<QString, QVariant> parameters;
};

//=================================================================================================
// WBackendUniversalPrivate
//=================================================================================================

class SK_BACKEND_EXPORT WBackendUniversalPrivate : public WBackendNetPrivate
{
public:
    WBackendUniversalPrivate(WBackendUniversal * p);

    void init(const QString & id, const QString & source);

public: // Functions
    void load();

    void runQuery(WBackendNetQuery * query, const QString & source, const QString & url) const;

    void applyQueryParameters(WBackendUniversalParameters * parameters,
                              const WBackendNetQuery      & query) const;

    void applyQueryResults(WBackendUniversalParameters * parameters,
                           WBackendNetQuery            * query) const;

    //---------------------------------------------------------------------------------------------

    void applySourceParameters(WBackendUniversalParameters * parameters,
                               const QByteArray            & data,
                               const WBackendNetQuery      & query,
                               const WBackendNetSource     & reply) const;

    void applySourceResults(WBackendUniversalParameters * parameters,
                            WBackendNetSource           * reply) const;

    //---------------------------------------------------------------------------------------------

    void applyTrackParameters(WBackendUniversalParameters * parameters,
                              const QByteArray            & data,
                              const WBackendNetQuery      & query,
                              const WBackendNetTrack      & reply) const;

    void applyTrackResults(WBackendUniversalParameters * parameters,
                           WBackendNetTrack            * reply) const;

    //---------------------------------------------------------------------------------------------

    void applyPlaylistParameters(WBackendUniversalParameters * parameters,
                                 const QByteArray            & data,
                                 const WBackendNetQuery      & query,
                                 const WBackendNetPlaylist   & reply) const;

    void applyPlaylistResults(WBackendUniversalParameters * parameters,
                              WBackendNetPlaylist         * reply) const;

    //---------------------------------------------------------------------------------------------

    void applyFolderParameters(WBackendUniversalParameters * parameters,
                               const QByteArray            & data,
                               const WBackendNetQuery      & query,
                               const WBackendNetFolder     & reply) const;

    void applyFolderResults(WBackendUniversalParameters * parameters,
                            WBackendNetFolder           * reply) const;

    //---------------------------------------------------------------------------------------------

    void applyItemParameters(WBackendUniversalParameters * parameters,
                             const QByteArray            & data,
                             const WBackendNetQuery      & query,
                             const WBackendNetItem       & reply) const;

    void applyItemResults(WBackendUniversalParameters * parameters,
                          WBackendNetItem             * reply) const;

    //---------------------------------------------------------------------------------------------

    void applyTrack(QList<WTrack>             * tracks, const QVariant & value) const;
    void applyItem (QList<WLibraryFolderItem> * items,  const QVariant & value) const;

    void applyQuery(WBackendNetQuery * query, QVariant * value) const;

    void applyQualities(QHash<WAbstractBackend::Quality, QString> * qualities,
                        QVariant                                  * value) const;

    //---------------------------------------------------------------------------------------------

    WAbstractBackend::Quality getQuality(const QString & string) const;

    QDateTime getDate(const QVariant & value) const;

    WLibraryItem::Type  getType (const QString & string) const;
    WLocalObject::State getState(const QString & string) const;

    const QVariant * getVariant(const QHash<QString, QVariant> * hash, const QString & name) const;

public: // Events
    void onLoaded();

    void onData(const WBackendUniversalData & data);

public: // Variables
    QThread * thread;

    WBackendUniversalData data;

    WRemoteData * remote;

    QString id;
    QString source;

    QMetaMethod method;

    QVariant global;

protected:
    W_DECLARE_PUBLIC(WBackendUniversal)
};

#endif // SK_NO_BACKENDUNIVERSAL
#endif // WBACKENDUNIVERSAL_P_H
