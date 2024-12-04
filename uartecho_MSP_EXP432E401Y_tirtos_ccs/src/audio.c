/*
* ======= audio.c =======
*/

//https://e2e.ti.com/support/microcontrollers/msp-low-power-microcontrollers-group/msp430/f/msp-low-power-microcontroller-forum/617912/boostxl-audio-real-time-audio-system-using-audio-signal-processing-boosterpack-and-msp432


// audio.c
#include "audio.h"
#include <ti/drivers/SPI.h>
#include "p100.h"  // Assuming glo and other global variables are declared here

const uint16_t SINETABLE[SINE_TABLE_SIZE+1] = {
 8192, 8393, 8594, 8795, 8995, 9195, 9394, 9593,
 9790, 9987, 10182, 10377, 10570, 10762, 10952, 11140,
 11327, 11512, 11695, 11875, 12054, 12230, 12404, 12575,
 12743, 12909, 13072, 13232, 13389, 13543, 13693, 13841,
 13985, 14125, 14262, 14395, 14525, 14650, 14772, 14890,
 15003, 15113, 15219, 15320, 15417, 15509, 15597, 15681,
 15760, 15835, 15905, 15971, 16031, 16087, 16138, 16185,
 16227, 16263, 16295, 16322, 16345, 16362, 16374, 16382,
 16383, 16382, 16374, 16362, 16345, 16322, 16295, 16263,
 16227, 16185, 16138, 16087, 16031, 15971, 15905, 15835,
 15760, 15681, 15597, 15509, 15417, 15320, 15219, 15113,
 15003, 14890, 14772, 14650, 14525, 14395, 14262, 14125,
 13985, 13841, 13693, 13543, 13389, 13232, 13072, 12909,
 12743, 12575, 12404, 12230, 12054, 11875, 11695, 11512,
 11327, 11140, 10952, 10762, 10570, 10377, 10182, 9987,
 9790, 9593, 9394, 9195, 8995, 8795, 8594, 8393,
 8192, 7991, 7790, 7589, 7389, 7189, 6990, 6791,
 6594, 6397, 6202, 6007, 5814, 5622, 5432, 5244,
 5057, 4872, 4689, 4509, 4330, 4154, 3980, 3809,
 3641, 3475, 3312, 3152, 2995, 2841, 2691, 2543,
 2399, 2259, 2122, 1989, 1859, 1734, 1612, 1494,
 1381, 1271, 1165, 1064, 967, 875, 787, 703,
 624, 549, 479, 413, 353, 297, 246, 199,
 157, 121, 89, 62, 39, 22, 10, 2,
 0, 2, 10, 22, 39, 62, 89, 121,
 157, 199, 246, 297, 353, 413, 479, 549,
 624, 703, 787, 875, 967, 1064, 1165, 1271,
 1381, 1494, 1612, 1734, 1859, 1989, 2122, 2259,
 2399, 2543, 2691, 2841, 2995, 3152, 3312, 3475,
 3641, 3809, 3980, 4154, 4330, 4509, 4689, 4872,
 5057, 5244, 5432, 5622, 5814, 6007, 6202, 6397,
 6594, 6791, 6990, 7189, 7389, 7589, 7790, 7991,
 8192
};

// Initialize audio system
// Make sure that the AMP_ON resistor on the audio boossterpack is set to R0!
void initAudio() {
    glo.audioController.lutPosition = 0.0;
    glo.audioController.lutDelta = 0.0;
    glo.audioController.setFreq = 0.0;

    // Initialize the SPI
    SPI_Params_init(&glo.audioController.audioSPIParams);
    //glo.audioSPIParams.bitRate = 4000000;  // Set SPI bit rate (e.g., 4 MHz)
    glo.audioController.audioSPIParams.dataSize = 16;       // 8-bit data size
    glo.audioController.audioSPIParams.frameFormat = SPI_POL0_PHA1;  // Adjust as per DAC requirements
    glo.audioController.audioSPI = SPI_open(CONFIG_SPI_0, &glo.audioController.audioSPIParams);  // CONFIG_SPI_0 is defined in .syscfg file
    if (glo.audioController.audioSPI == NULL) {
        // SPI_open() failed
        //System_abort("Failed to open SPI");
        while(1);
    }

    // Additional DAC initialization
    digitalWrite(4, LOW);   // Set PK5 LOW to enable audio amplifier on BOOSTXL-AUDIO
    digitalWrite(5, HIGH);  // Set PD4 LOW to enable microphone on BOOSTXL-AUDIO
}

void initADCBuf(){
    ADCBuf_init();
    ADCBuf_Params_init(&glo.audioController.adcBufParams);
    glo.audioController.adcBufParams.returnMode = ADCBuf_RETURN_MODE_CALLBACK;
    glo.audioController.adcBufParams.recurrenceMode = ADCBuf_RECURRENCE_MODE_CONTINUOUS;

    glo.audioController.adcBufParams.callbackFxn = adcBufCallback;
    glo.audioController.adcBufParams.samplingFrequency = 8000;
    glo.audioController.adcBuf = ADCBuf_open(CONFIG_ADCBUF_0, &glo.audioController.adcBufParams);
    if (glo.audioController.adcBuf == NULL) {
        // ADCBuf_open() failed
        while(1);
    }

    glo.audioController.adcConversion.adcChannel = ADCBUF_CHANNEL_0;
    glo.audioController.adcConversion.arg = NULL;
    glo.audioController.adcConversion.sampleBuffer = glo.audioController.
}

// Generate sine sample and send to DAC
void generateSineSample() {
    uint32_t l_index, u_index;
    double l_weight, u_weight;
    double answer;
    uint16_t outval;
    SPI_Transaction spiTransaction;
    bool transferOK;

    // Check if sine wave generation is active
    if (glo.audioController.lutDelta == 0.0) {
        return;  // No sine wave to generate
    }

    // Calculate indices for interpolation
    l_index = (uint32_t) glo.audioController.lutPosition;
    u_index = (l_index + 1) % SINE_TABLE_SIZE;
    if (u_index >= SINE_TABLE_SIZE) {
        u_index = 0;  // Wrap around
    }

    // Calculate interpolation weights
    u_weight = glo.audioController.lutPosition - (double) l_index;
    l_weight = 1.0 - u_weight;

    // Perform linear interpolation
    answer = (double) SINETABLE[l_index] * l_weight + (double) SINETABLE[u_index] * u_weight;
    outval = (uint16_t)(answer + 0.5);  // Round to nearest integer


    spiTransaction.count = 1;
    // spiTransaction.txBuf = (void *) &outval;
    // spiTransaction.rxBuf = (void *) NULL;
    spiTransaction.txBuf = &outval;
    spiTransaction.rxBuf = NULL;


    // Send data to DAC via SPI
    transferOK = SPI_transfer(glo.audioController.audioSPI, &spiTransaction);
    if (!transferOK) {
        // Handle SPI transfer error if necessary
        // For now, we can ignore or log the error
        while(1); // Infinite loop to halt the system
    }

    // Update the phase accumulator
    glo.audioController.lutPosition += glo.audioController.lutDelta;
    while (glo.audioController.lutPosition >= (double) SINE_TABLE_SIZE) {
        glo.audioController.lutPosition -= (double) SINE_TABLE_SIZE;
    }
}

// void sine(char buffer) {
//     uint32_t freq;
//     double l_weight, u_weight;
//     uint32_t l_index, u_index;
//     double answer;
//     uint16_t outval;
//     SPI_Transaction spiTrans;
//     bool transferCheck;
//     char outBuffer[MSG_SIZE];

//     if(global.timer0.period == 0) { addPayload("-print Timer 0 is off"); return; }
//     if(buffer && buffer[0] != 0 && global.timer0.period > 0) {
//         if(isdigit(buffer) == 0) {
//             sprintf(outBuffer, "-print message %s missing required digits", buffer);
//             throwError(3, outBuffer);
//         }
//         freq = atoi(buffer);
//         global.lutController.lutDelta = (double)freq * (double)LUT_SIZE * (double)global.timer0.period / 1000000.;
//     }
//     if(global.lutController.lutDelta >= LUT_SIZE / 2) {
//         global.lutController.lutDelta = 0.;
//         addPayload("-print Nyquist violation");
//         timer("0");
//         return;
//     }
//     if(global.lutController.lutDelta <= 0) {
//         global.lutController.lutDelta = 0.;
//         if(global.timer0.period > 0) {
//             timer("0");
//             callback("0 0");
//         }
//         addPayload("-print Timer 0 is off");
//         return;
//     }
//     if(buffer) return;
//     l_index = (uint32_t) global.lutController.lutPosition;
//     u_index = l_index + 1;
//     u_weight = global.lutController.lutPosition - (double) l_index;
//     l_weight = 1. - u_weight;
//     answer = (double) global.lutController.sinlut14[l_index] * l_weight + (double) global.lutController.sinlut14[u_index] * u_weight;
//     outval = round(answer);

//     spiTrans.count = 1;
//     spiTrans.txBuf = (void ) &outval;
//     spiTrans.rxBuf = (void) NULL;

//     transferCheck = SPI_transfer(global.spi, &spiTrans);
//     if (!transferCheck) while(1);

//     global.lutController.lutPosition += global.lutController.lutDelta;
//     while(global.lutController.lutPosition >= (double) LUT_SIZE) global.lutController.lutPosition -= (double) LUT_SIZE;
// }
