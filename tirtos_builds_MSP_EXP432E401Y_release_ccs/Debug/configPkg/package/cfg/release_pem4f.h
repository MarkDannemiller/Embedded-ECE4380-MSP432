/*
 *  Do not modify this file; it is automatically 
 *  generated and any modifications will be overwritten.
 *
 * @(#) xdc-I11
 */

#include <xdc/std.h>

#include <ti/sysbios/heaps/HeapMem.h>
extern const ti_sysbios_heaps_HeapMem_Handle heap0;

#include <ti/sysbios/gates/GateSwi.h>
extern const ti_sysbios_gates_GateSwi_Handle gateSwi0;

#include <ti/sysbios/knl/Task.h>
extern const ti_sysbios_knl_Task_Handle UARTReader0;

#include <ti/sysbios/knl/Task.h>
extern const ti_sysbios_knl_Task_Handle UARTWriter;

#include <ti/sysbios/knl/Semaphore.h>
extern const ti_sysbios_knl_Semaphore_Handle UARTWriteSem;

#include <ti/sysbios/knl/Task.h>
extern const ti_sysbios_knl_Task_Handle PayloadExecutor;

#include <ti/sysbios/knl/Queue.h>
extern const ti_sysbios_knl_Queue_Handle PayloadQueue;

#include <ti/sysbios/knl/Queue.h>
extern const ti_sysbios_knl_Queue_Handle OutMsgQueue;

#include <ti/sysbios/knl/Semaphore.h>
extern const ti_sysbios_knl_Semaphore_Handle PayloadSem;

#include <ti/sysbios/knl/Swi.h>
extern const ti_sysbios_knl_Swi_Handle Timer0_swi;

#include <ti/sysbios/knl/Swi.h>
extern const ti_sysbios_knl_Swi_Handle SW1_swi;

#include <ti/sysbios/knl/Swi.h>
extern const ti_sysbios_knl_Swi_Handle SW2_swi;

#include <ti/sysbios/knl/Semaphore.h>
extern const ti_sysbios_knl_Semaphore_Handle TickerSem;

#include <ti/sysbios/knl/Task.h>
extern const ti_sysbios_knl_Task_Handle TickerProcessor;

#include <ti/sysbios/knl/Task.h>
extern const ti_sysbios_knl_Task_Handle UARTReader1;

#include <ti/sysbios/gates/GateSwi.h>
extern const ti_sysbios_gates_GateSwi_Handle gateSwi1;

#include <ti/sysbios/gates/GateSwi.h>
extern const ti_sysbios_gates_GateSwi_Handle gateSwi2;

#include <ti/sysbios/gates/GateSwi.h>
extern const ti_sysbios_gates_GateSwi_Handle gateSwi3;

#include <ti/sysbios/knl/Semaphore.h>
extern const ti_sysbios_knl_Semaphore_Handle ADCSemaphore;

#include <ti/sysbios/knl/Task.h>
extern const ti_sysbios_knl_Task_Handle ADCStreamer;

#include <ti/sysbios/gates/GateMutex.h>
extern const ti_sysbios_gates_GateMutex_Handle tiposix_mqGate;

extern int xdc_runtime_Startup__EXECFXN__C;

extern int xdc_runtime_Startup__RESETFXN__C;

extern int xdc_rov_runtime_Mon__checksum;

extern int xdc_rov_runtime_Mon__write;

