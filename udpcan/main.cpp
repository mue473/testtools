/* ======================================================================= */
/*   simple qt-based program for udp-encapsulated CAN message handling     */
/*   (c) 2020 Rainer Müller                                                */
/* ======================================================================= */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QApplication>

#include "udpcan.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    UdpCan udpcan;
    udpcan.show();
    return app.exec();
}
