/*
 * tasks.h
 *
 *  Created on: Sep 27, 2024
 *      Author: Mark Dannemiller
 */

#ifndef SRC_TASKS_H_
#define SRC_TASKS_H_

#define NUM_TASKS 4  // Update with the number of tasks below

void uart0ReadTask(UArg arg0, UArg arg1);            // Handles incoming UART 0 messages
void uartWriteTask(UArg arg0, UArg arg1);           // Handles outgoing UART 0 messages
void uart0ReadTask(UArg arg0, UArg arg1);           // Handles incoming UART 1 messages
void executePayloadTask(UArg arg0, UArg arg1);      // Executes device functions based on payloads from PayloadQueue
void tickerProcessingTask(UArg arg0, UArg arg1);    // Processes tickers periodically from tickers[] array
void ADCStream();                                  // Streams ADC data to UART 1

#endif /* SRC_TASKS_H_ */



