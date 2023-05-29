/**
 * @file heartmonitor.h
 * @brief Header file for Heart Monitoring functions
 * @author Burkov Egor
 * @date 2022-03-17
 */


#ifndef INC_HEARTMONITOR_H_
#define INC_HEARTMONITOR_H_

#define HR_DEBUG

/**
 * @brief HeartMonitor debug mode
 *
 * Store intermediate values of processing of signals.
 */
#ifdef HR_DEBUG

/**
 * @brief Size of the debug arrays.
 *
 * This constant defines the size of the debug arrays used for HR debugging.
 * It specifies the maximum number of elements that each debug array can hold.
 */
#define HR_SIZE 25

/**
 * @brief Debug arrays for HR module.
 *
 * These arrays are used for debugging purposes in the HR module.
 * Each array has a size of HR_SIZE.
 */
extern float d1[HR_SIZE], d2[HR_SIZE], d3[HR_SIZE], d4[HR_SIZE],
             d5[HR_SIZE], d6[HR_SIZE], d7[HR_SIZE], d8[HR_SIZE];

#endif
/**
 * @brief Structure representing a Band-Pass Filter (BPF) filter.
 */
typedef struct {
	int n; /**< Number of elements */
	float freq; /**< Center frequency */
	float freq_low; /**< Lower cutoff frequency */
	float freq_high; /**< Upper cutoff frequency */
	float *A, *d1, *d2, *d3, *d4, *w0, *w1, *w2, *w3, *w4; /**< Pointers to coefficients */
} BPF_filter;

void BPF_process(BPF_filter *filter, float *array, int array_size);
void BPF_free(BPF_filter *bpf_filter);
BPF_filter* BPF_new(int order, float freq, float low, float high);

/**
 * @brief Structure representing a Moving Average (MA) filter.
 */
typedef struct {
	int window_size; /**< Size of the moving average window */
	float *data; /**< Pointer to the previous data array */
} MA_filter;

MA_filter* MA_new(int window_size);
void MA_process(MA_filter *filter, float *array, int array_size);
void MA_free(MA_filter *filter);

/**
 * @brief Structure representing a Slope Sum Function (SSF).
 */
typedef struct {
	int window_size; /**< Window size */
	float *data; /**< Pointer to the data array */
} SSF;

SSF* SSF_new(int window_size);
void SSF_free(SSF *ssf);
void SSF_process(SSF *ssf, float *array, int array_size);

/**
 * @brief Structure representing the preprocessing stage for heart rate estimation using green channel data.
 */
typedef struct {
	BPF_filter *bpf_green; /**< Pointer to a Band-Pass Filter (BPF) for green channel data */
	SSF *ssf_green; /**< Pointer to a Simple Smoothing Filter (SSF) for green channel data */
	MA_filter *ma_green;
} HR_GreenPreprocess;

HR_GreenPreprocess* HR_greenPreprocess_new(float freq, int bpf_order,
		float bpf_low, float bpf_high, int ssf_size);
void HR_greenPreprocess_free(HR_GreenPreprocess *greenPrepocess);
void HR_greenPreprocess_process(HR_GreenPreprocess *greenPrepocess,
		float *array, int array_size);

/**
 * @brief Structure representing a Heart Monitor for heart rate estimation.
 */
typedef struct {
    int size;                          /**< Size of the data arrays */
    float freq;                        /**< Frequency of the heart monitor */
    float *green;                      /**< Pointer to the green channel data array */
    float *red;                        /**< Pointer to the red channel data array */
    float *ir;                         /**< Pointer to the infrared channel data array */
    int *peaks;                        /**< Pointer to the peaks array */
    float threshold;                   /**< Threshold value for peak detection */
    HR_GreenPreprocess *greenPreprocess; /**< Pointer to the green channel preprocessing stage */
} HR_HeartMonitor;

HR_HeartMonitor* HR_heartMonitor_new(float freq, int size, float threshold,
		int bpf_order, float bpf_low, float bpf_high, int ssf_size,
		int ma_green_size, int ma_redIr_size);
void HR_heartMonitor_free(HR_HeartMonitor *heartMonitor);

void HR_heartMonitor_addGreen(HR_HeartMonitor *heartMonitor, float *array,
		int array_size);
void HR_heartMonitor_addRedIr(HR_HeartMonitor *heartMonitor, float *array_red,
		float *array_ir, int array_size);

void HR_heartMonitor_peaksFromGreen(HR_HeartMonitor *heartMonitor);
float HR_heartMonitor_heartRateFromPeaks(HR_HeartMonitor *heartMonitor);
float HR_heartMonitor_ratioFromPeaks(HR_HeartMonitor *heartMonitor);
float HR_heartMonitor_ratioFromIndexedPeaks(HR_HeartMonitor *heartMonitor,
		int *peaks, int peaks_size);
float HR_heartMonitor_heartRateFromIndexedPeaks(HR_HeartMonitor *heartMonitor,
		int *peaks, int peaks_size);

void peaks_detect(float *array, int *signals, int array_size,
		float initial_threshold);
void peaks_normalize(float *array, int *signals, int array_size);
void peaks_indexing(int *signals, int *ret_signals, int array_size);
int* peaks_indexingSize(int *signals, int array_size, int *result_size);

#endif /* INC_HEARTMONITOR_H_ */
