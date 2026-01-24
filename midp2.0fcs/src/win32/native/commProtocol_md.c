/*
 * @(#)commProtocol_md.c	1.19 02/07/24 @(#)
 *
 * Copyright (c) 2000-2002 Sun Microsystems, Inc.  All rights reserved.
 * PROPRIETARY/CONFIDENTIAL
 * Use is subject to license terms.
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: networking
 * FILE:      commProtocol_md.c
 * OVERVIEW:  Operations to support serial communication ports on Windows
 *            (native Win32 support for the 'comm:' protocol)
 * AUTHOR:    Stephen Flores
 *=======================================================================*/

/*=======================================================================
 * Include files
 *=======================================================================*/

#include <windows.h>
#include <winbase.h>
#include <stdio.h>

#include <commProtocol.h>

/*=======================================================================
 * Definitions and declarations
 *=======================================================================*/

static char* getLastErrorMsg(char* pHeader);

/* non system errors */
static char* COMM_BAD_PORTNUMBER = "Invalid port number";
static char* COMM_BAD_BAUDRATE   = "Invalid baud rate";

/*=======================================================================
 * Protocol implementation functions
 *=======================================================================*/

void freePortError(char* pszError) {
    if (NULL == pszError ||
        COMM_BAD_BAUDRATE == pszError) {
        return;
    }

    // Free the buffer allocated by FormatMessage.
    LocalFree(pszError);
}

long openPortByNumber(char** ppszError, long port, long baudRate,
                      long options) {

    char* lpFileName = NULL;

    switch (port) {
    case 0:
        lpFileName = "COM1:";
        break;

    case 1:
        lpFileName = "COM2:";
        break;

    case 2:
        lpFileName = "COM3:";
        break;

    case 3:
        lpFileName = "COM4:";
        break;

    case 4:
        lpFileName = "COM5:";
        break;

    case 5:
        lpFileName = "COM6:";
        break;

    case 6:
        lpFileName = "COM7:";
        break;

    case 7:
        lpFileName = "COM8:";
        break;

    default:
        *ppszError = COMM_BAD_PORTNUMBER;
        return -1;
    }

    return openPortByName(ppszError, lpFileName, baudRate, options);
}

long openPortByName(char** ppszError, char* pszDeviceName, long baudRate,
                    long options) {

    HANDLE hPort = 0;
    DWORD dwDesiredAccess = GENERIC_READ | GENERIC_WRITE; 
    DWORD dwShareMode = 0;
    LPSECURITY_ATTRIBUTES lpSecurityAttributes = NULL;
    DWORD dwCreationDistribution = OPEN_EXISTING;
    DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
    HANDLE hTemplateFile = 0;

    hPort = CreateFile(pszDeviceName, dwDesiredAccess, dwShareMode,
        lpSecurityAttributes, dwCreationDistribution, dwFlagsAndAttributes,
        hTemplateFile);
    if (INVALID_HANDLE_VALUE == hPort) {
        *ppszError = getLastErrorMsg("cannot open port");
        return -1;
    }

    configurePort(ppszError, (int)hPort, baudRate, options);
    if (*ppszError != NULL) {
        CloseHandle(hPort);
        return -1;
    }

    return (int)hPort;
}

void configurePort(char** ppszError, int hPort, long baudRate,
                          unsigned long options) {

    DCB deviceControlBlock;
    COMMTIMEOUTS commTimeouts;

    memset(&deviceControlBlock, 0, sizeof (deviceControlBlock));
    deviceControlBlock.DCBlength = sizeof (deviceControlBlock);
    
    switch (baudRate) {
    case 110:
        deviceControlBlock.BaudRate = CBR_110;
        break;

    case 300:
        deviceControlBlock.BaudRate = CBR_300;
        break;

    case 600:
        deviceControlBlock.BaudRate = CBR_600;
        break;

    case 1200:
        deviceControlBlock.BaudRate = CBR_1200;
        break;

    case 2400:
        deviceControlBlock.BaudRate = CBR_2400;
        break;

    case 4800:
        deviceControlBlock.BaudRate = CBR_4800;
        break;

    case 9600:
        deviceControlBlock.BaudRate = CBR_9600;
        break;

    case 14400:
        deviceControlBlock.BaudRate = CBR_14400;
        break;

    case 19200:
        deviceControlBlock.BaudRate = CBR_19200;
        break;

    case 38400:
        deviceControlBlock.BaudRate = CBR_38400;
        break;

    case 56000:
        deviceControlBlock.BaudRate = CBR_56000;
        break;

    case 57600:
        deviceControlBlock.BaudRate = CBR_57600;
        break;

    case 115200:
        deviceControlBlock.BaudRate = CBR_115200;
        break;

    case 128000:
        deviceControlBlock.BaudRate = CBR_128000;
        break;

    case 256000:
        deviceControlBlock.BaudRate = CBR_256000;
        break;

    default:
        *ppszError = COMM_BAD_BAUDRATE;
        return;
    }

    /* binary mode, no EOF check */
    deviceControlBlock.fBinary = 1;

    /* 0-4=no,odd,even,mark,space */
    if (options & ODD_PARITY) {
        deviceControlBlock.fParity = 1;
        deviceControlBlock.Parity = 1;
    } else if (options & EVEN_PARITY) {
        deviceControlBlock.fParity = 1;
        deviceControlBlock.Parity = 2;
    }

    /* CTS output flow control */
    if (options & AUTO_CTS) {
        deviceControlBlock.fOutxCtsFlow = 1;
    }

    deviceControlBlock.fDtrControl = DTR_CONTROL_ENABLE;

    /* RTS flow control */
    if (options & AUTO_RTS) {
        deviceControlBlock.fRtsControl = RTS_CONTROL_HANDSHAKE;
    } else {
        deviceControlBlock.fRtsControl = RTS_CONTROL_ENABLE;
    }

    /* BITS_PER_CHAR_8 is 2 bits and includes BITS_PER_CHAR_7 */
    if ((options & BITS_PER_CHAR_8) == BITS_PER_CHAR_8) {
        deviceControlBlock.ByteSize = 8;
    } else {
        deviceControlBlock.ByteSize = 7;
    }

    /* StopBits 0,1,2 = 1, 1.5, 2 */
    if (options & STOP_BITS_2) {
        deviceControlBlock.StopBits = 2;
    }

    if (!SetCommState((HANDLE)hPort, &deviceControlBlock)) {
        *ppszError = getLastErrorMsg("cannot set port state");
        return;
    }
  
    if (!GetCommTimeouts((HANDLE)hPort, &commTimeouts)) {
        *ppszError = getLastErrorMsg("cannot get port timeouts");
        return;
    }
        
    /* set non blocking since we are not using async methods */
    commTimeouts.ReadIntervalTimeout = MAXDWORD;
    commTimeouts.ReadTotalTimeoutMultiplier = 0;
    commTimeouts.ReadTotalTimeoutConstant = 10;

    commTimeouts.WriteTotalTimeoutMultiplier = 0;
    commTimeouts.WriteTotalTimeoutConstant = 10;
        
    if (!SetCommTimeouts((HANDLE)hPort, &commTimeouts)) {
        *ppszError = getLastErrorMsg("cannot set port timeouts");
        return;
    }

    *ppszError = NULL;
}

void closePort(long hPort) {
    CloseHandle((HANDLE)hPort);
}

/*=========================================================================
 * FUNCTION:        writeToPort
 * OVERVIEW:        Writes to the open port, without blocking
 * INTERFACE:
 *   parameters:    **ppszError:            Error message
 *                  hPort:                  The name of the port to write to.
 *                  *pBuffer:               The data to be written to the port.
 *                  nNumberOfBytesToWrite:  The number of bytes to write.
 *   returns:       The number of bytes written.
 *=======================================================================*/

long writeToPort(char** ppszError, long hPort, char* pBuffer,
                 long nNumberOfBytesToWrite) {

    DWORD nNumberOfBytesWritten = 0;
    LPOVERLAPPED lpOverlapped = NULL;

    if (!WriteFile((HANDLE)hPort, pBuffer, nNumberOfBytesToWrite,
            &nNumberOfBytesWritten, lpOverlapped)) {
        *ppszError = getLastErrorMsg("write failed");
        return -1;
    }

    *ppszError = NULL;
    return nNumberOfBytesWritten;
}

/*=========================================================================
 * FUNCTION:        readFromPort
 * OVERVIEW:        reads from a serial port, without blocking
 * INTERFACE:
 *   parameters:    **ppszError:          Error message
 *                  hPort:                The name of the port to read from.
 *                  pBuffer:              The buffer to store the bytes read.
 *                  nNumberOfBytesToRead: The number of bytes to read.
 *   returns:       The number of bytes read.
 *=======================================================================*/

long readFromPort(char** ppszError, long hPort, char* pBuffer,
                 long nNumberOfBytesToRead) {

    DWORD nNumberOfBytesRead = 0;
    LPOVERLAPPED lpOverlapped = NULL;

    if (!ReadFile((HANDLE)hPort, pBuffer, nNumberOfBytesToRead,
            &nNumberOfBytesRead, lpOverlapped)) {
        *ppszError = getLastErrorMsg("read failed");
        return -1;
    }

    *ppszError = NULL;
    return nNumberOfBytesRead;
}

static char* getLastErrorMsg(char* pHeader) {

    DWORD lastError;
    char* lpMsgBuf;

    lastError = GetLastError();
    FormatMessage( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        lastError,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR)&lpMsgBuf,
        0,
        NULL);

#if INCLUDEDEBUGCODE

    printf("%s(%08.8X)", pHeader, lastError);
   
    // Display the string.
    MessageBox(NULL, lpMsgBuf, "GetLastError",
               MB_OK | MB_ICONINFORMATION);

#endif /* INCLUDEDEBUGCODE */

    return lpMsgBuf;
}


