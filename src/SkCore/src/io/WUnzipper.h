//=================================================================================================
/*
    Copyright (C) 2015-2017 Sky kit authors united with omega. <http://omega.gg/about>

    Author: Benjamin Arnaud. <http://bunjee.me> <bunjee@omega.gg>

    This file is part of the SkCore module of Sky kit.

    - GNU General Public License Usage:
    This file may be used under the terms of the GNU General Public License version 3 as published
    by the Free Software Foundation and appearing in the LICENSE.md file included in the packaging
    of this file. Please review the following information to ensure the GNU General Public License
    requirements will be met: https://www.gnu.org/licenses/gpl.html.
*/
//=================================================================================================

#ifndef WUNZIPPER_H
#define WUNZIPPER_H

// Qt includes
#ifdef QT_4
#include <QStringList>
#else
#include <QObject>
#endif

// Sk includes
#include <Sk>

class WUnzipperPrivate;

class SK_CORE_EXPORT WUnzipper : public QObject, public WPrivatable
{
    Q_OBJECT

    Q_PROPERTY(QString fileName READ fileName WRITE setFileName NOTIFY fileNameChanged)

    Q_PROPERTY(QString destination READ destination WRITE setDestination NOTIFY destinationChanged)

    Q_PROPERTY(QStringList fileNames READ fileNames WRITE setFileNames NOTIFY fileNamesChanged)

public:
    explicit WUnzipper(QObject * parent = NULL);

public: // Interface
    Q_INVOKABLE void extract();

    Q_INVOKABLE void extract(const QString     & fileName,
                             const QString     & destination,
                             const QStringList & fileNames = QStringList());

public: // Static functions
    Q_INVOKABLE static QStringList getFileNames(const QString & fileName);

signals:
    void fileNameChanged();

    void destinationChanged();

    void fileNamesChanged();

public: // Properties
    QString fileName() const;
    void    setFileName(const QString & fileName);

    QString destination() const;
    void    setDestination(const QString & destination);

    QStringList fileNames() const;
    void        setFileNames(const QStringList & fileNames);

private:
    W_DECLARE_PRIVATE(WUnzipper)
};

#endif // WUNZIPPER_H
