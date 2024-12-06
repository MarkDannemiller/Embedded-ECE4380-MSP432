#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>
#include <ti/drivers/SPI.h>
#include <ti/drivers/ADCBuf.h>


#define SINE_TABLE_SIZE 256
#define DATABLOCKSIZE 128
#define DATADELAY 8
#define TXBUFCOUNT 2

// Sine table provided
extern const uint16_t SINETABLE[SINE_TABLE_SIZE+1];

// For receiving data from the BOOSTXL-AUDIO Board
typedef struct ADCBufControl{
    ADCBuf_Conversion conversion;
    uint16_t *RX_Completed;
    uint32_t converting;
    uint32_t ping_count;
    uint32_t pong_count;
    uint16_t RX_Ping[DATABLOCKSIZE];
    uint16_t RX_Pong[DATABLOCKSIZE];
} ADCBufControl;

// For transmitting data to the BOOSTXL-AUDIO Board using -sine command
typedef struct TXBufControl{
    uint16_t *TX_Completed;
    int32_t  TX_index;
    int32_t  TX_correction;
    uint32_t TX_delay;
    uint32_t TX_sample_count;
    uint16_t TX_Ping[DATABLOCKSIZE];
    uint16_t TX_Pong[DATABLOCKSIZE];
} TXBufControl;

// Structure to hold audio control variables and SPI handle
typedef struct {
    // SPI Handle for communication with BOOSTXL-AUDIO Board
    SPI_Handle audioSPI;
    SPI_Params audioSPIParams;

    volatile double lutPosition;    // Current position in the sine lookup table
    volatile double lutDelta;       // Delta value to increment the position
    volatile double setFreq;        // Desired frequency for the sine wave

    ADCBuf_Params adcBufParams;
    ADCBuf_Handle adcBuf;

    ADCBufControl adcBufControl;            // ADC buffer control structure
    TXBufControl txBufControl[TXBUFCOUNT];  // TX buffer control structures
} AudioController;

// Function declarations
void initAudio();
void initADCBuf();
void generateSineSample();

#endif /* AUDIO_H_ */
