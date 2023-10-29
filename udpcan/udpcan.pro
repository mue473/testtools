QT += network widgets
requires(qtConfig(udpsocket))

HEADERS       = udpcan.h
SOURCES       = udpcan.cpp \
                main.cpp
