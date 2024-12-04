#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>
#include <ti/drivers/SPI.h>


#define SINE_TABLE_SIZE 256

// Sine table provided
extern const uint16_t SINETABLE[SINE_TABLE_SIZE+1];

// Structure to hold audio control variables and SPI handle
typedef struct {
    // SPI Handle for communication with BOOSTXL-AUDIO Board
    SPI_Handle audioSPI;
    SPI_Params audioSPIParams;

    volatile double lutPosition;    // Current position in the sine lookup table
    volatile double lutDelta;       // Delta value to increment the position
    volatile double setFreq;        // Desired frequency for the sine wave
} AudioController;

// Function declarations
void initAudio();
void generateSineSample();

#endif /* AUDIO_H_ */