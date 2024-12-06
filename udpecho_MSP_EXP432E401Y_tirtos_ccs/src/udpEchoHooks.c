/*
 * udpEchoHooks.c - Adapted to user environment
 * Hooks for IP address changes and service reports.
 */

#include <stdint.h>
#include <stdio.h>

/* POSIX Header files */
#include <pthread.h>

/* NDK includes */
#include <ti/ndk/inc/netmain.h>
#include <ti/ndk/slnetif/slnetifndk.h>
#include <ti/net/slnet.h>
#include <ti/net/slnetif.h>
#include <ti/net/slnetutils.h>
#include <ti/drivers/emac/EMACMSP432E4.h>

/* User code includes */
#include "p100.h" // For glo, AddProgramMessage, etc.

#define UDPPORT 1000
#define UDPHANDLERSTACK 5120
#define IFPRI  4

extern void *ListenFxn(void *arg0);
extern void *TransmitFxn(void *arg0);

void netIPAddrHook(uint32_t IPAddr, unsigned int IfIdx, unsigned int fAdd)
{
    pthread_t thread;
    pthread_attr_t attrs;
    struct sched_param priParam;
    int retc;
    int detachState;
    uint32_t hostByteAddr;
    static uint16_t arg0 = UDPPORT;
    static bool createTask = true;
    int32_t status = 0;
    char MsgBuffer[128];

    if (fAdd) {
        sprintf(MsgBuffer, "Network Added: ");
        AddProgramMessage(MsgBuffer);
    } else {
        sprintf(MsgBuffer, "Network Removed: ");
        AddProgramMessage(MsgBuffer);
    }

    hostByteAddr = NDK_ntohl(IPAddr);
    sprintf(MsgBuffer,"If-%d:%d.%d.%d.%d\n", IfIdx,
            (uint8_t)(hostByteAddr>>24)&0xFF,
            (uint8_t)(hostByteAddr>>16)&0xFF,
            (uint8_t)(hostByteAddr>>8)&0xFF,
            (uint8_t)hostByteAddr&0xFF);
    AddProgramMessage(MsgBuffer);

    status = ti_net_SlNet_initConfig();
    if (status < 0) {
        AddProgramMessage("Error: Failed to initialize SlNet interface(s).\r\n");
        while (1);
    }

    if (fAdd && createTask) {
                /*
         *  Create the Task that handles incoming UDP packets.
         *  arg0 will be the port that this task listens to.
         */

        /*
         * Create Listening Thread
         */
        pthread_attr_init(&attrs);
        priParam.sched_priority = 1;

        detachState = PTHREAD_CREATE_DETACHED;
        retc = pthread_attr_setdetachstate(&attrs, detachState);
        if (retc != 0) {
            AddProgramMessage("Error: pthread_attr_setdetachstate() failed.\r\n");
            while (1);
        }

        pthread_attr_setschedparam(&attrs, &priParam);
        retc |= pthread_attr_setstacksize(&attrs, UDPHANDLERSTACK);
        if (retc != 0) {
            AddProgramMessage("Error: pthread_attr_setstacksize() failed.\r\n");
            while (1);
        }

        retc = pthread_create(&thread, &attrs, ListenFxn, (void *)&arg0);
        if (retc != 0) {
            AddProgramMessage("Error: ListenFxn pthread_create() failed.\r\n");
            while (1);
        }

        retc = pthread_create(&thread, &attrs, TransmitFxn, (void *)&arg0);
        if (retc != 0) {
            AddProgramMessage("Error: TransmitFxn pthread_create() failed.\r\n");
            while (1);
        }

        createTask = false;
    }
}

/*
 *  ======== serviceReport ========
 *  NDK service report.  Initially, just reports common system issues.
 */
void serviceReport(uint32_t item, uint32_t status, uint32_t report, void *h)
{
    static char *taskName[] = {"Telnet", "", "NAT", "DHCPS", "DHCPC", "DNS"};
    static char *reportStr[] = {"", "Running", "Updated", "Complete", "Fault"};
    static char *statusStr[] = {"Disabled", "Waiting", "IPTerm", "Failed","Enabled"};
    char MsgBuffer[128];

    sprintf(MsgBuffer, "Service Status: %-9s: %-9s: %-9s: %03d\n",
            taskName[item - 1], statusStr[status], reportStr[report / 256],
            report & 0xFF);
    AddProgramMessage(MsgBuffer);

    if ((item == CFGITEM_SERVICE_DHCPCLIENT) &&
        (status == CIS_SRV_STATUS_ENABLED) &&
        (report & NETTOOLS_STAT_FAULT)) {
        AddProgramMessage("DHCP Client initialization failed; check your network.\n");
        //while (1);
    }
}
