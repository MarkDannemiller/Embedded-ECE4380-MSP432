/*
 * udpEcho.c - Adapted to user environment
 * BSD Sockets code for listening and transmitting UDP messages.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

/* Include TI-RTOS NDK BSD headers */
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>

#include <ti/net/slnetutils.h>

/* Include your user globals and functions */
#include "p100.h"  // For glo, AddProgramMessage, raiseError, etc.

#define UDPPACKETSIZE 1472
#define MAXPORTLEN    6
#define DEFAULT_NET_PORT 1000

typedef struct _my_ip_mreq {
    struct in_addr imr_multiaddr;
    struct in_addr imr_interface;
} my_ip_mreq;

extern void fdOpenSession(void *taskHandle);
extern void fdCloseSession(void *taskHandle);
extern void *TaskSelf(void);

extern void ParseVoice(char *ch);
extern void ParseNetUDP(char *ch, int32_t binaryCount);
extern bool MatchSubString(const char *needle, const char *haystack);

// Replace AddError(...) with AddProgramMessage("Error: ...\r\n")
// Replace AddPayload(... with -print) with AddProgramMessage(...)

/*static */char *UDPParse(char *buff, struct sockaddr_in *clientAddr, bool todash) {
    char *StrBufPTR;
    char *colon;
    int32_t AddrByte;
    uint32_t PortWord;

    StrBufPTR = buff;
    if(!StrBufPTR) return StrBufPTR;

    if(MatchSubString("localhost", StrBufPTR)){
        clientAddr->sin_addr.s_addr = 0x0100007F;
        goto coloncheck;
    }
    if(MatchSubString("broadcast", StrBufPTR)){
        clientAddr->sin_addr.s_addr = 0xFFFFFFFF;
        goto coloncheck;
    }
    if(MatchSubString("nobody", StrBufPTR)){
        clientAddr->sin_addr.s_addr = 0x00000000;
        goto coloncheck;
    }

    if(isdigit((int)*StrBufPTR) == 0){
        AddProgramMessage("Error: Message Missing Required Digits.\r\n");
        return NULL;
    }
    AddrByte = atoi(StrBufPTR);
    clientAddr->sin_addr.s_addr = AddrByte;

    StrBufPTR = strchr(StrBufPTR, '.');
    if(!StrBufPTR) return StrBufPTR;
    StrBufPTR++;
    if(*StrBufPTR == 0) return NULL;
    AddrByte = atoi(StrBufPTR);
    clientAddr->sin_addr.s_addr |= AddrByte << 8;

    StrBufPTR = strchr(StrBufPTR, '.');
    if(!StrBufPTR) return StrBufPTR;
    StrBufPTR++;
    if(*StrBufPTR == 0) return NULL;
    AddrByte = atoi(StrBufPTR);
    clientAddr->sin_addr.s_addr |= AddrByte << 16;

    StrBufPTR = strchr(StrBufPTR, '.');
    if(!StrBufPTR) return StrBufPTR;
    StrBufPTR++;
    if(*StrBufPTR == 0) return NULL;
    AddrByte = atoi(StrBufPTR);
    clientAddr->sin_addr.s_addr |= AddrByte << 24;

coloncheck:
    colon = strchr(StrBufPTR, ':');
    if(!colon) {
        PortWord = DEFAULT_NET_PORT;
        StrBufPTR = strchr(StrBufPTR, ' ');
    } else {
        StrBufPTR = colon;
        StrBufPTR++;
        if(*StrBufPTR == 0) return NULL;
        if(isdigit((int)*StrBufPTR) == 0) return NULL;
        PortWord = (uint32_t)atoi(StrBufPTR);
    }

    clientAddr->sin_port = htons((uint16_t)PortWord);
    clientAddr->sin_family = AF_INET;

    if(StrBufPTR) {
        if(todash)
            StrBufPTR = strchr(StrBufPTR, '-');
        else
            while(StrBufPTR && isspace((int)*StrBufPTR))
                StrBufPTR++;
    }
    return StrBufPTR;
}

void *ListenFxn(void *arg0)
{
    int bytesRcvd;
    int status;
    int server = -1;
    fd_set readSet;
    struct addrinfo hints;
    struct addrinfo *res = NULL, *p = NULL;
    struct sockaddr_in clientAddr;
    socklen_t addrlen;
    char buffer[UDPPACKETSIZE+1]; // +1 for null terminator
    char portNumber[MAXPORTLEN];
    char MsgBuff[320];
    int32_t optval = 1;
    my_ip_mreq mreq;
    uint16_t listeningPort = *(uint16_t *)arg0;

    fdOpenSession(TaskSelf());

    sprintf(portNumber, "%u", (unsigned)listeningPort);
    sprintf(MsgBuff, "UDP Recv started : %s\r\n", portNumber);
    AddProgramMessage(MsgBuff);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags    = AI_PASSIVE;

    status = getaddrinfo(NULL, portNumber, &hints, &res);
    if (status != 0) {
        AddProgramMessage("Error: getaddrinfo() failed.\r\n");
        goto shutdown;
    }

    for (p = res; p != NULL; p = p->ai_next) {
        server = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (server == -1) {
            continue;
        }

        status = bind(server, p->ai_addr, p->ai_addrlen);
        if (status != -1) {
            break;
        }

        close(server);
    }

    if (server == -1) {
        AddProgramMessage("Error: Socket Not Created.\r\n");
        goto shutdown;
    } else if (p == NULL) {
        AddProgramMessage("Error: Bind failed.\r\n");
        goto shutdown;
    } else {
        freeaddrinfo(res);
        res = NULL;
    }

    mreq.imr_multiaddr.s_addr = htonl(INADDR_ANY);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    setsockopt(server, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
    setsockopt(server, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    do {
        FD_ZERO(&readSet);
        FD_SET(server, &readSet);
        addrlen = sizeof(clientAddr);

        status = select(server + 1, &readSet, NULL, NULL, NULL);

        if (status > 0) {
            if (FD_ISSET(server, &readSet)) {
                bytesRcvd = (int)recvfrom(server, buffer, UDPPACKETSIZE, 0,
                                         (struct sockaddr *)&clientAddr, &addrlen);

                if (bytesRcvd > 0) {
                    buffer[bytesRcvd] = '\0';
                    if(MatchSubString("-voice", buffer))
                        ParseVoice(buffer);
                    else {
                        sprintf(MsgBuff, "UDP %d.%d.%d.%d> %s\r\n",
                                (uint8_t)(clientAddr.sin_addr.s_addr      & 0xFF),
                                (uint8_t)((clientAddr.sin_addr.s_addr>> 8)&0xFF),
                                (uint8_t)((clientAddr.sin_addr.s_addr>>16)&0xFF),
                                (uint8_t)((clientAddr.sin_addr.s_addr>>24)&0xFF),
                                buffer);
                        AddProgramMessage(MsgBuff);
                        // Add message to queue if needed or just processed above
                        // If needed:
                        AddPayload(buffer);
                    }
                }
            }
        }
    } while (status > 0);

shutdown:
    if (res) {
        freeaddrinfo(res);
    }

    if (server != -1) {
        close(server);
    }

    fdCloseSession(TaskSelf());
    return (NULL);
}

void *TransmitFxn(void *arg0)
{
    int bytesSent;
    int status;
    int server = -1;
    fd_set writeSet;
    struct sockaddr_in clientAddr;
    socklen_t addrlen;
    char *StrBufPTR;
    int allow_broadcast = 1;
    uint32_t gateKey;
    int payloadnext;
    int bytesRequested;

    fdOpenSession(TaskSelf());

    AddProgramMessage("UDP Transmit started.\r\n");

    // Just create a socket for sending
    server = socket(AF_INET, SOCK_DGRAM, 0);
    if (server == -1) {
        AddProgramMessage("Error: Socket not created.\r\n");
        goto shutdown;
    }

    status = setsockopt(server, SOL_SOCKET, SO_BROADCAST, (void*) &allow_broadcast, sizeof(allow_broadcast));
    if(status < 0){
        AddProgramMessage("Error: setsockopt SO_BROADCAST failed.\r\n");
    }

    while(1){
        // Wait for a payload to send
        Semaphore_pend(glo.bios.NetSemaphore, BIOS_WAIT_FOREVER);

        FD_ZERO(&writeSet);
        FD_SET(server, &writeSet);

        bytesSent = -1;
        gateKey = GateSwi_enter(gateSwi4);

        // Assume glo.NetOutQ structure and usage similar to example code
        StrBufPTR = UDPParse(glo.NetOutQ.payloads[glo.NetOutQ.payloadReading], &clientAddr, true);
        if(StrBufPTR){
            bytesRequested = (int)strlen(StrBufPTR) + 1;
            bytesRequested += glo.NetOutQ.binaryCount[glo.NetOutQ.payloadReading];

            bytesSent = (int)sendto(server, StrBufPTR, bytesRequested, 0,
                                    (struct sockaddr *)&clientAddr, sizeof(clientAddr));
        }

        if(!StrBufPTR) {
            AddProgramMessage("Error: UDP Parse Failed.\r\n");
        } else if(bytesSent < 0 || bytesSent != bytesRequested) {
            AddProgramMessage("Error: Sendto() failed.\r\n");
        }

        payloadnext = glo.NetOutQ.payloadReading + 1;
        if(payloadnext >= NetQueueLen) // define NetQueueLen in global
            payloadnext = 0;
        glo.NetOutQ.payloadReading = payloadnext;
        GateSwi_leave(gateSwi4, gateKey);
    }

shutdown:
    if (server != -1) {
        close(server);
    }

    fdCloseSession(TaskSelf());
    return (NULL);
}
