//=================================================================================================
/*
    Copyright (C) 2015-2017 Sky kit authors united with omega. <http://omega.gg/about>

    Author: Benjamin Arnaud. <http://bunjee.me> <bunjee@omega.gg>

    This file is part of the tools for Sky kit.

    - GNU General Public License Usage:
    This file may be used under the terms of the GNU General Public License version 3 as published
    by the Free Software Foundation and appearing in the LICENSE.md file included in the packaging
    of this file. Please review the following information to ensure the GNU General Public License
    requirements will be met: https://www.gnu.org/licenses/gpl.html.
*/
//=================================================================================================

// Qt includes
#include <QCoreApplication>
#include <QTextStream>
#include <QDir>

//-------------------------------------------------------------------------------------------------
// Global variables

QString version;

QString defineA;
QString defineB;

//-------------------------------------------------------------------------------------------------
// Functions
//-------------------------------------------------------------------------------------------------

void replaceFile(const QString & path, const QString & content)
{
    qDebug("Generating file: %s", qPrintable(path));

    QFile file(path);

    file.open(QIODevice::WriteOnly | QIODevice::Truncate);

    QTextStream stream(&file);

    stream << content;
}

//-------------------------------------------------------------------------------------------------

bool skipLines(QTextStream * stream, const QString & string)
{
    QString line = stream->readLine();

    if (line.isNull()) return false;

    while (line.startsWith(string) == false)
    {
        line = stream->readLine();

        if (line.isNull()) return false;
    }

    return true;
}

void scanFile(const QString & input, const QString & output)
{
    QString content;

    QFile file(input);

    file.open(QIODevice::ReadOnly);

    QTextStream stream(&file);

    QString line = stream.readLine();

    while (line.isNull() == false)
    {
        if (line.startsWith("import "))
        {
            line = line.simplified();

            if (line.startsWith("import QtQuick"))
            {
                content.append("import QtQuick " + version + '\n');
            }
        }
        else if (line.startsWith(defineA))
        {
            line = stream.readLine();

            if (line.isNull()) break;

            while (line.startsWith("//#") == false)
            {
                if (line.isNull())
                {
                    replaceFile(output, content);

                    return;
                }
                else content.append(line + '\n');

                line = stream.readLine();
            }

            if (line.startsWith("//#ELSE"))
            {
                if (skipLines(&stream, "//#") == false) break;
            }
        }
        else if (line.startsWith(defineB))
        {
            if (skipLines(&stream, "//#") == false) break;
        }
        else if (line.startsWith("//#") == false)
        {
            content.append(line + '\n');
        }

        line = stream.readLine();
    }

    replaceFile(output, content);
}

//-------------------------------------------------------------------------------------------------

void scanFolder(const QString & path, const QString & output)
{
    QDir Dir(path);

    QFileInfoList list = Dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);

    foreach (QFileInfo info, list)
    {
        if (info.suffix().toLower() != "qml") continue;

        QString filePath = info.filePath();

        scanFile(filePath, info.path() + '/' + output + '/' + info.fileName());
    }
}

//-------------------------------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);

    if (argc != 3)
    {
        qDebug("Usage: gmlGenerator <path> <output> <version>");

        return -1;
    }

    qDebug("QML deployer");

    qDebug("\nDEPLOYING");

    version = argv[3];

    if (version.startsWith("1."))
    {
        defineA = "//#QT_4";
        defineB = "//#QT_5";
    }
    else
    {
        defineA = "//#QT_5";
        defineB = "//#QT_4";
    }

    scanFolder(argv[1], argv[2]);

    qDebug("DONE");
}