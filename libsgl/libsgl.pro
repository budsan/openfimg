#-------------------------------------------------
#
# Project created by QtCreator 2011-06-21T10:37:51
#
#-------------------------------------------------

QT       -= core gui

TARGET = libsgl
TEMPLATE = lib

DEFINES += LIBSGL_LIBRARY

SOURCES += \
    glesTex.cpp \
    glesPixel.cpp \
    glesMatrix.cpp \
    glesGet.cpp \
    glesBase.cpp \
    fglmatrix.cpp \
    eglMem.cpp \
    eglBase.cpp \
    libfimg/texture.c \
    libfimg/system.c \
    libfimg/shaders.c \
    libfimg/raster.c \
    libfimg/primitive.c \
    libfimg/host.c \
    libfimg/global.c \
    libfimg/fragment.c \
    libfimg/dump.c \
    libfimg/compat.c \
    glesFrame.cpp

HEADERS += \
    types.h \
    state.h \
    s3c_g2d.h \
    glesCommon.h \
    fgltextureobject.h \
    fglsurface.h \
    fglstack.h \
    fglobjectmanager.h \
    fglobject.h \
    fglmatrix.h \
    fglbufferobject.h \
    eglMem.h \
    common.h \
    libfimg/s3c_g3d.h \
    libfimg/fimg_private.h \
    libfimg/fimg.h \
    libfimg/config.h \
    libfimg/shaders/vert.h \
    libfimg/shaders/frag.h \
    fglframebufferobject.h

symbian {
    #Symbian specific definitions
    MMP_RULES += EXPORTUNFROZEN
    TARGET.UID3 = 0xE5B54922
    TARGET.CAPABILITY = 
    TARGET.EPOCALLOWDLLDATA = 1
    addFiles.sources = libsgl.dll
    addFiles.path = !:/sys/bin
    DEPLOYMENT += addFiles
}

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/local/lib
    }
    INSTALLS += target
}

OTHER_FILES += \
    libfimg/shaders/vert.asm \
    libfimg/shaders/frag.asm
