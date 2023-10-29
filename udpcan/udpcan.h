/* ======================================================================= */
/*   simple qt-based program for udp-encapsulated CAN message handling     */
/*   (c) 2020 - 2022 by Rainer Müller                                      */
/* ======================================================================= */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef UDPCAN_H
#define UDPCAN_H

#include <QWidget>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QUdpSocket>


class UdpCan : public QWidget
{
    Q_OBJECT

public:
    explicit UdpCan(QWidget *parent = nullptr);
    void addTraceLine(QString trace);
    void transmitDatagram(QByteArray datagram);

private slots:
    void PingPressed();
    void QueryPressed();
    void PwOnPressed();
    void PwOffPressed();
    void QConfPressed();
    void processRxDatagrams();

private:
    QPushButton *pbPing;
    QPushButton *pbQuery;
    QPushButton *pbPwOn;
    QPushButton *pbPwOff;
    QPushButton *pbQConf;
    QListWidget *lvTrace;
    QCheckBox   *cbListSc;
    QUdpSocket  *udpSocket = nullptr;
};

#endif
