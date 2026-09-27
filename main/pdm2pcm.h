/**
 * @file pdm2pcm.h
 * @brief Generic CIC helpers and application PDM-to-PCM filters.
 *
 * @author Levy Gabriel da S. G.
 * @date July 1 2022
 */

#ifndef _PDM2PCM_H_
#define _PDM2PCM_H_

#ifndef C_POSIX_LIB_INCLUDED
    #define C_POSIX_LIB_INCLUDED
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <stdint.h>
    #include <inttypes.h>
    #include <math.h>
#endif //C_POSIX_LIB_INCLUDED


#define SAMPLES 1022 
#define BYTE_PER_SAMPLE 4
#define APPLY_MASK(x,i) (int32_t)((((x>>(31-i))&0x00000001) << 1)-1)
#define INPUT_SAMPLE_SIZE 32
#define FIR_ORDER 64

#define MAX_OVERFLOW  1073741823 // (int32_t)(pow(2,30)-1)
#define MIN_OVERFLOW -1073741824 // (int32_t)(-pow(2,30))

/** Signed 16-bit output sample used by the generic CIC helper. */
typedef short sample_t;

/** Heap-backed FIFO state for the generic comb helper. */
typedef struct {
    size_t head, tail, size;
    size_t capacity;
    int32_t* data;
} fifo_t;

/** Accumulator state for a generic CIC integrator. */
typedef struct 
{
    int32_t acc;
} integrator_t;

/** Delay state for a generic CIC comb. */
typedef struct 
{
    int32_t previous;
    int32_t actual;
    int32_t diff;
    char delay;
    fifo_t fifo;
} comb_t;

/** Stage parameters and allocated state for a generic CIC filter. */
typedef struct 
{
    int N; // stages
    int R; // decimation factor
    int M; // differential delay
    int G; // gain
    integrator_t *integrators;
    comb_t *combs;
    int count;
} cic_t;

/** Two-stage CIC state used by the recorder. */
typedef struct 
{
    int32_t acc_s1;
    int32_t acc_s2;
    int32_t prev_s1;
    int32_t prev_s2;
} app_cic_t;

/** Sample history for the optional coefficient-based FIR. */
typedef struct {
    short kernel[FIR_ORDER];
} app_fir_t;


/**
 * @brief Initialize FIFO structure with default parameters.
 *
 * @param fifo Pointer to FIFO strcuture to be initialized.
 * @param size FIFO capacity in 32-bit elements; must be positive.
 * @warning Allocation failure is not checked, and there is no matching free helper.
 */
void init_fifo(fifo_t *fifo, size_t size);

/**
 * @brief Enqueue data into FIFO.
 *
 * @param fifo Pointer to FIFO structure to be modified.
 * @param data_in Value to append; silently discarded when the FIFO is full.
 */
void enqueue(fifo_t *fifo, int32_t data_in);

/**
 * @brief Dequeue data from FIFO.
 *
 * @param fifo Pointer to FIFO structure to be accessed.
 * @param[out] data_out Receives the removed value; unchanged if the FIFO is empty.
 */
void dequeue(fifo_t *fifo, int32_t *data_out);

/**
 * @brief Initialize Integrator structure with default parameters.
 *
 * @param integ Pointer to Integrator structure to be initialized.
 */
void init_integrator(integrator_t *integ);

/**
 * @brief Initialize Comb structure with default parameters.
 *
 * @param comb Pointer to Comb structure to be initialized.
 * @param delay Differential delay and FIFO capacity; must be positive.
 * @warning Allocation failure is not checked.
 */
void init_comb(comb_t *comb, char delay);

/**
 * @brief Initialize CIC structure with default parameters.
 *
 * @param cic Pointer to CIC structure to be initialized.
 * @param N Number of Integrator/Comb stages.
 * @param R Decimation rate.
 * @param M Differential delay; must be positive.
 * @warning Allocations are unchecked and no teardown helper is provided.
 */
void init_cic(cic_t *cic, int N, int R, int M);

/**
 * @brief Process one step in Integrator structure.
 *
 * @param integ Pointer to Integrator structure to be modified.
 * @param data_in Data to be processed.
 * @return Updated accumulator value.
 */
int32_t process_integrator(integrator_t *integ, int32_t data_in);

/**
 * @brief Process one step in Comb structure.
 *
 * @param comb Pointer to Comb structure to be modified.
 * @param data_in Data to be processed.
 * @return Difference between the input and delayed value.
 */
int32_t process_comb(comb_t *comb, int32_t data_in);

/**
 * @brief Process one step in CIC structure.
 *
 * @param cic Pointer to CIC structure to be modified.
 * @param data_in Data to be processed.
 * @param[out] data_available Set to 1 at a decimation output, otherwise 0.
 * @return Current output cast to sample_t; use only when data_available is 1.
 */
sample_t process_cic(cic_t *cic, int32_t data_in, char *data_available);

/**
 * @brief Swap bytes of word (int32_t).
 * 
 * @param x 32-bit word to reorder.
 * @return Word with byte order reversed.
 */
int32_t swap_bytes_of_word(int32_t x);

/**
 * @brief Initialize App Specific CIC structure with default parameters.
 *
 * @param cic Pointer to App Specific CIC structure to be initialized.
 */
void init_app_cic(app_cic_t *cic);

/**
 * @brief Initialize App Specific FIR structure with default parameters.
 *
 * @param fir Pointer to App Specific FIR structure to be initialized.
 */
void init_app_fir(app_fir_t *fir);

/**
 * @brief Check for overflow in integrators' accumulators for App Specific CIC structure.
 *
 * @param cic Pointer to App Specific CIC structure to be used.
 */
void integ_overflow(app_cic_t *cic);

/**
 * @brief Process input buffer with App Specific CIC structure.
 *
 * @param cic Persistent CIC state.
 * @param input_buffer Array of SAMPLES packed 32-bit PDM words.
 * @param output_buffer Array receiving 2*SAMPLES signed short samples.
 */
void process_app_cic(app_cic_t *cic, int32_t (*input_buffer)[SAMPLES], short (*output_buffer)[2*SAMPLES]);

/**
 * @brief Process input buffer with App Specific FIR structure.
 *
 * @param fir Persistent FIR history.
 * @param fir_coeffs Array of FIR_ORDER coefficients.
 * @param pcm_samples Buffer of 2*SAMPLES samples modified in place.
 */
void process_app_fir(app_fir_t *fir, short fir_coeffs[FIR_ORDER], short (*pcm_samples)[2*SAMPLES]);

/** @brief Apply the three-term post-filter used by the storage task.
 * @param pcm_samples Buffer of 2*SAMPLES samples modified in place.
 * @warning Static state is shared across calls and recordings; calls must be serialized.
 */
void process_new_fir(short (*pcm_samples)[2*SAMPLES]);

#endif // _PDM2PCM_H_