#! /usr/bin/env python3

from smartcard.scard import *
import smartcard.util

SET_CARD_TYPE = [0x20, 0x11, 1, 1, 0, 0]

# Read data: 0xFF 0xB0 'Address MSB' 'Address LSB' Length
COMMAND = [0xFF, 0xB0, 0x00, 0x00, 0x10]

try:
    hresult, hcontext = SCardEstablishContext(SCARD_SCOPE_USER)
    if hresult != SCARD_S_SUCCESS:
        raise Exception('Failed to establish context : ' + SCardGetErrorMessage(hresult))
    print('Context established!')

    try:
        hresult, readers = SCardListReaders(hcontext, [])
        if hresult != SCARD_S_SUCCESS:
            raise Exception('Failed to list readers: ' + SCardGetErrorMessage(hresult))
        print('PCSC Readers:', readers)

        if len(readers) < 1:
            raise Exception('No smart card readers')

        reader = readers[0]
        print("Using reader:", reader)

        try:
            hresult, hcard, dwActiveProtocol = SCardConnect(hcontext, reader, SCARD_SHARE_DIRECT, 0)
            if hresult != SCARD_S_SUCCESS:
                raise Exception('Unable to connect: ' + SCardGetErrorMessage(hresult))
            print('Connected with active protocol', dwActiveProtocol)

            try:
                hresult, response = SCardControl(hcard, dwActiveProtocol, SET_CARD_TYPE)
                if hresult != SCARD_S_SUCCESS:
                    raise Exception('Failed to set card type: ' + SCardGetErrorMessage(hresult))
                print('Set card type response: ' + smartcard.util.toHexString(response, smartcard.util.HEX))
                hresult, response = SCardTransmit(hcard, dwActiveProtocol, COMMAND)
                if hresult != SCARD_S_SUCCESS:
                    raise Exception('Failed to read: ' + SCardGetErrorMessage(hresult))
                print('Read response: ' + smartcard.util.toHexString(response, smartcard.util.HEX))
            finally:
                hresult = SCardDisconnect(hcard, SCARD_UNPOWER_CARD)
                if hresult != SCARD_S_SUCCESS:
                    raise Exception('Failed to disconnect: ' + SCardGetErrorMessage(hresult))
                print('Disconnected')

        except Exception as message:
            print("Exception:", message)

    finally:
        hresult = SCardReleaseContext(hcontext)
        if hresult != SCARD_S_SUCCESS:
            raise Exception('Failed to release context: ' + SCardGetErrorMessage(hresult))
        print('Released context.')

except Exception as message:
    print("Exception:", message)

import sys
if 'win32' == sys.platform:
    print('press Enter to continue')
    sys.stdin.read(1)
