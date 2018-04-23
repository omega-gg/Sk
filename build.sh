#!/bin/sh

#--------------------------------------------------------------------------------------------------
# Settings
#--------------------------------------------------------------------------------------------------

Sky="../.."

external="/c/dev/workspace/3rdparty"

#--------------------------------------------------------------------------------------------------

Qt4_version="4.8.7"
Qt5_version="5.10.1"

MinGW_version="5.3.0"

#--------------------------------------------------------------------------------------------------

make_arguments="-j 4"

#--------------------------------------------------------------------------------------------------
# Linux

lib64="/lib/x86_64-linux-gnu"

#--------------------------------------------------------------------------------------------------
# Syntax
#--------------------------------------------------------------------------------------------------

if [ $# != 2 -a $# != 3 ] \
   || \
   [ $1 != "qt4" -a $1 != "qt5" -a $1 != "clean" ] \
   || \
   [ $2 != "win32" -a $2 != "osx" -a $2 != "linux" ] \
   || \
   [ $# = 3 -a "$3" != "deploy" ]; then

    echo "Usage: build <qt4 | qt5 | clean> <win32 | osx | linux> [deploy]"

    exit 1
fi

#--------------------------------------------------------------------------------------------------
# Configuration
#--------------------------------------------------------------------------------------------------

if [ $1 = "qt4" ]; then

    Qt="$external/Qt/$Qt4_version"
else
    Qt="$external/Qt/$Qt5_version"
fi

MinGW="$external/MinGW/$MinGW_version/bin"

#--------------------------------------------------------------------------------------------------
# Clean
#--------------------------------------------------------------------------------------------------

if [ $1 = "clean" ]; then

    echo "CLEANING"

    rm -rf build/qt4/*
    rm -rf build/qt5/*

    touch build/qt4/.gitignore
    touch build/qt5/.gitignore

    exit 0
fi

#--------------------------------------------------------------------------------------------------
# Building Sky
#--------------------------------------------------------------------------------------------------

echo "BUILDING Sky"
echo "------------"

if [ $1 = "qt4" ]; then

    QT_SELECT=qt4

    build="build/qt4"
else
    QT_SELECT=qt5

    build="build/qt5"
fi

if [ $2 = "win32" ]; then

    spec=win32-g++

    PATH="$Qt/bin:$MinGW:$PATH"

elif [ $2 = "osx" ]; then

    spec=macx-g++

    export PATH=/usr/local/opt/qt\@5.5/bin/:$PATH

    export LIBRARY_PATH=/usr/local/lib:/usr/local/opt/openssl/lib

elif [ $2 = "linux" ]; then

    if [ -d "/usr/lib/x86_64-linux-gnu" ]; then

        spec=linux-g++-64
    else
        spec=linux-g++-32
    fi
fi

qmake --version
echo ""

cd $build

qmake -r -spec $spec "CONFIG += release" $Sky
echo ""

if [ $2 = "win32" ]; then

    mingw32-make $make_arguments
else
    make $make_arguments
fi

echo "------------"

#--------------------------------------------------------------------------------------------------
# Deploying Sky
#--------------------------------------------------------------------------------------------------

if [ "$3" = "deploy" ]; then

    echo ""
    echo "DEPLOYING Sky"
    echo "-------------"

    cd $Sky

    sh deploy.sh $1 $2

    echo "-------------"
fi
