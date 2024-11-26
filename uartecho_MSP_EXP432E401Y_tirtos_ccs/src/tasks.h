/*
 * tasks.h
 *
 *  Created on: Sep 27, 2024
 *      Author: Mark Dannemiller
 */

#ifndef SRC_TASKS_H_
#define SRC_TASKS_H_

#define NUM_TASKS 4  // Update with the number of tasks below

void uartReadTask(UArg arg0, UArg arg1);
void uartWriteTask(UArg arg0, UArg arg1);
void executePayloadTask(UArg arg0, UArg arg1);
void tickerProcessingTask(UArg arg0, UArg arg1);


#endif /* SRC_TASKS_H_ */



