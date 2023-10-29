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

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTime>

#include "udpcan.h"

uint32_t be32(uchar *u) {
    return (u[0] << 24) | (u[1] << 16) | (u[2] << 8) | u[3];
}


UdpCan::UdpCan(QWidget *parent) : QWidget(parent)
{
    setFixedSize(680, 365);
    pbPing = new QPushButton("ping request", this);
    pbPing->setGeometry(QRect(20, 20, 100, 27));
    pbQuery = new QPushButton("query power", this);
    pbQuery->setGeometry(QRect(20, 60, 100, 27));
    pbPwOn = new QPushButton("power on", this);
    pbPwOn->setGeometry(QRect(20, 100, 100, 27));
    pbPwOff = new QPushButton("power off", this);
    pbPwOff->setGeometry(QRect(20, 140, 100, 27));
    pbQConf = new QPushButton("query conf", this);
    pbQConf->setGeometry(QRect(20, 180, 100, 27));
    lvTrace = new QListWidget(this);
    lvTrace->setGeometry(QRect(150, 10, 510, 335));
    lvTrace->setFont(QFont("Courier"));
    cbListSc = new QCheckBox("autoscroll", this);
    cbListSc->setGeometry(QRect(50, 320, 100, 27));

   	connect(pbPing,  SIGNAL(clicked()), this, SLOT(PingPressed()));
   	connect(pbQuery, SIGNAL(clicked()), this, SLOT(QueryPressed()));
   	connect(pbPwOn,  SIGNAL(clicked()), this, SLOT(PwOnPressed()));
   	connect(pbPwOff, SIGNAL(clicked()), this, SLOT(PwOffPressed()));
   	connect(pbQConf, SIGNAL(clicked()), this, SLOT(QConfPressed()));

    udpSocket = new QUdpSocket(this);
    udpSocket->bind(15730, QUdpSocket::ShareAddress);
    connect(udpSocket, &QUdpSocket::readyRead,
            this, &UdpCan::processRxDatagrams);

    setWindowTitle("UDP-CAN");
    addTraceLine(tr("Listening for UDP messages"));
}


void UdpCan::addTraceLine(QString trace)
{
    QTime t = QTime::currentTime();
    QString dat = t.toString("hh:mm:ss.zzz  ");
    dat.append(trace);
    QListWidgetItem *newItem = new QListWidgetItem;
    newItem->setText(dat);
    lvTrace->addItem(newItem);
    if (cbListSc->isChecked()) lvTrace->scrollToItem(newItem);
}


void UdpCan::transmitDatagram(QByteArray datagram)
{
    uint8_t dlc = datagram.data()[4];
    datagram.append(8 - dlc, 0);
    if (udpSocket->writeDatagram(datagram, QHostAddress::Broadcast, 15731) != 13)
        addTraceLine(QString("Transmit error"));
    else {    
        uint32_t canid = be32((uchar *)datagram.data());
        QString rdat = QString("TX %1 L=%2:")
            .arg(canid, 8, 16, QChar('0')).arg(dlc);
        for (int i=0; i<dlc; i++) {
            uint8_t d = datagram.data()[i+5];
            rdat.append(QString(" %1").arg(d, 2, 16, QChar('0')));
        }
        addTraceLine(rdat);
    }
}


void UdpCan::PingPressed()
{
    transmitDatagram(QByteArray::fromHex("0030030100"));
}

void UdpCan::QueryPressed()
{
    transmitDatagram(QByteArray::fromHex("000003010400000000"));
    pbQuery->setStyleSheet("background: lightgray");
}

void UdpCan::PwOnPressed()
{
    transmitDatagram(QByteArray::fromHex("00000301050000000001"));
}

void UdpCan::PwOffPressed()
{
    transmitDatagram(QByteArray::fromHex("00000301050000000000"));
}

void UdpCan::QConfPressed()
{
    transmitDatagram(QByteArray::fromHex("0040af7e00"));
}


void UdpCan::processRxDatagrams()
{
    QByteArray datagram;
    datagram.resize(16);

    while (udpSocket->hasPendingDatagrams()) {
        udpSocket->readDatagram(datagram.data(), datagram.size());
        uint32_t canid = be32((uchar *)datagram.data());
        uint8_t dlc = datagram.data()[4];
        QString rdat = QString("RX %1 L=%2:")
            .arg(canid, 8, 16, QChar('0')).arg(dlc);
        for (int i=0; i<dlc; i++) {
            uint8_t d = datagram.data()[i+5];
            rdat.append(QString(" %1").arg(d, 2, 16, QChar('0')));
        }
        addTraceLine(rdat);
        
        switch ((canid & 0x01FF0000UL) >> 16) {
            case 0x01:  if (dlc == 5) switch (datagram.data()[9]) {
                    case 0: pbQuery->setStyleSheet("background-color: red; border: none");
                            break;
                    case 1: pbQuery->setStyleSheet("background-color: green; border: none");
                            break;
                        }
                        break;
        }
    }
}
