/**
 * @file heartmonitor.c
 * @brief Heart Monitoring functions
 * @author Burkov Egor
 * @date 2022-03-17
 */

#include <heartmonitor.h>
#include <math.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef HR_DEBUG

float d1[HR_SIZE];
float d2[HR_SIZE];
float d3[HR_SIZE];
float d4[HR_SIZE];
float d5[HR_SIZE];
float d6[HR_SIZE];
float d7[HR_SIZE];
float d8[HR_SIZE];

#endif

int findMaxIndex(float *array, int start, int end);
int findMinIndex(float *array, int start, int end);

/**
 * @brief Creates a new HR_GreenPreprocess structure.
 *
 * This function creates and initializes a new HR_GreenPreprocess structure for preprocessing the
 * green channel data in the heart rate estimation process. It allocates memory for the structure
 * and initializes the Band-Pass Filter (BPF) and Slope Sum Function (SSF) within it.
 *
 * @param freq       Frequency of the heart monitor.
 * @param bpf_order  Order of the Band-Pass Filter (BPF).
 * @param bpf_low    Lower cutoff frequency of the BPF.
 * @param bpf_high   Upper cutoff frequency of the BPF.
 * @param ssf_size   Size of the Slope Sum Function (SSF).
 *
 * @return Pointer to the newly created HR_GreenPreprocess structure.
 */
HR_GreenPreprocess* HR_greenPreprocess_new(float freq, int bpf_order,
		float bpf_low, float bpf_high, int ssf_size) {
	SSF *ssf = SSF_new(ssf_size);
	BPF_filter *bpf = BPF_new(bpf_order, freq, bpf_low, bpf_high);
	MA_filter *ma = MA_new(7);
	HR_GreenPreprocess *greenPreprocess = (HR_GreenPreprocess*) malloc(
			sizeof(HR_GreenPreprocess));
	greenPreprocess->bpf_green = bpf;
	greenPreprocess->ssf_green = ssf;
	greenPreprocess->ma_green = ma;
	return greenPreprocess;
}

/**
 * @brief Frees the memory associated with HR_GreenPreprocess structure.
 *
 * This function frees the memory allocated for the HR_GreenPreprocess structure and its associated
 * Band-Pass Filter (BPF) and Slope Sum Function (SSF).
 *
 * @param greenPreprocess Pointer to the HR_GreenPreprocess structure to be freed.
 * @return None.
 */
void HR_greenPreprocess_free(HR_GreenPreprocess *greenPrepocess) {
	BPF_free(greenPrepocess->bpf_green);
	SSF_free(greenPrepocess->ssf_green);
	MA_free(greenPrepocess->ma_green);
	free(greenPrepocess);
}

/**
 * @brief Processes the green channel data using the HR_GreenPreprocess structure.
 *
 * This function processes the green channel data using the provided HR_GreenPreprocess structure.
 * It applies Band-Pass Filtering (BPF) and Slope Sum Function (SSF) to the data array.
 *
 * @param greenPreprocess Pointer to the HR_GreenPreprocess structure.
 * @param array           Pointer to the green channel data array.
 * @param array_size      Size of the green channel data array.
 * @return None.
 */
void HR_greenPreprocess_process(HR_GreenPreprocess *greenPrepocess,
		float *array, int array_size) {
	BPF_process(greenPrepocess->bpf_green, array, array_size);
#ifdef HR_DEBUG
	memcpy(d1, array, HR_SIZE * sizeof(float));
#endif
	SSF_process(greenPrepocess->ssf_green, array, array_size);
	MA_process(greenPrepocess->ma_green, array, array_size);
#ifdef HR_DEBUG
	memcpy(d2, array, HR_SIZE * sizeof(float));
#endif
}

/**
 * @brief Creates a new HR_HeartMonitor structure.
 *
 * This function creates and initializes a new HR_HeartMonitor structure for heart rate monitoring.
 * It allocates memory for the structure and its associated data arrays, initializes the parameters,
 * and creates an HR_GreenPreprocess structure for preprocessing the green channel data.
 *
 * @param freq           Frequency of the heart monitor.
 * @param size           Size of the data arrays.
 * @param threshold      Threshold value for peak detection.
 * @param bpf_order      Order of the Band-Pass Filter (BPF).
 * @param bpf_low        Lower cutoff frequency of the BPF.
 * @param bpf_high       Upper cutoff frequency of the BPF.
 * @param ssf_size       Size of the Slope Sum Function (SSF).
 * @param ma_green_size  Size of the Moving Average filter for the green channel.
 * @param ma_redIr_size  Size of the Moving Average filter for the red and infrared channels.
 *
 * @return Pointer to the newly created HR_HeartMonitor structure.
 */
HR_HeartMonitor* HR_heartMonitor_new(float freq, int size, float threshold,
		int bpf_order, float bpf_low, float bpf_high, int ssf_size,
		int ma_green_size, int ma_redIr_size) {

	HR_GreenPreprocess *greenPreprocess = HR_greenPreprocess_new(freq,
			bpf_order, bpf_low, bpf_high, ssf_size);

	HR_HeartMonitor *heartMonitor = (HR_HeartMonitor*) malloc(
			sizeof(HR_HeartMonitor));
	heartMonitor->size = size;
	heartMonitor->freq = freq;
	heartMonitor->green = (float*) calloc(size, sizeof(float));
	heartMonitor->red = (float*) calloc(size, sizeof(float));
	heartMonitor->ir = (float*) calloc(size, sizeof(float));
	heartMonitor->peaks = (int*) calloc(size, sizeof(int));
	heartMonitor->threshold = threshold;
	heartMonitor->greenPreprocess = greenPreprocess;
	return heartMonitor;
}

/**
 * @brief Frees the memory associated with HR_HeartMonitor structure.
 *
 * This function frees the memory allocated for the HR_HeartMonitor structure and its associated
 * data arrays, as well as the HR_GreenPreprocess structure used for preprocessing.
 *
 * @param heartMonitor Pointer to the HR_HeartMonitor structure to be freed.
 * @return None.
 */
void HR_heartMonitor_free(HR_HeartMonitor *heartMonitor) {
	HR_greenPreprocess_free(heartMonitor->greenPreprocess);
	free(heartMonitor->green);
	free(heartMonitor->red);
	free(heartMonitor->ir);
	free(heartMonitor);
}

/**
 * @brief Adds green channel data to the HR_HeartMonitor structure.
 *
 * This function adds the green channel data to the HR_HeartMonitor structure. It processes the
 * data using the HR_GreenPreprocess structure, updates the internal green channel data array.
 *
 * @param heartMonitor Pointer to the HR_HeartMonitor structure.
 * @param array        Pointer to the green channel data array.
 * @param array_size   Size of the green channel data array.
 * @return None.
 */
void HR_heartMonitor_addGreen(HR_HeartMonitor *heartMonitor, float *array,
		int array_size) {
	HR_greenPreprocess_process(heartMonitor->greenPreprocess, array,
			array_size);
	if (array_size >= heartMonitor->size) {
		memcpy(heartMonitor->green, &array[array_size - heartMonitor->size],
				heartMonitor->size * sizeof(float));
	} else {
		memmove(heartMonitor->green, &heartMonitor->green[array_size],
				(heartMonitor->size - array_size) * sizeof(float));
		memmove(&heartMonitor->green[heartMonitor->size - array_size], array,
				array_size * sizeof(float));
	}

}

/**
 * @brief Adds red and infrared channel data to the HR_HeartMonitor structure.
 *
 * This function adds the red and infrared channel data to the HR_HeartMonitor structure. It handles cases
 * where the array size is smaller than the size of the HR_HeartMonitor data array, and updates the internal
 * red and infrared channel data arrays accordingly.
 *
 * @param heartMonitor Pointer to the HR_HeartMonitor structure.
 * @param array_red    Pointer to the red channel data array.
 * @param array_ir     Pointer to the infrared channel data array.
 * @param array_size   Size of the red and infrared channel data arrays.
 * @return None.
 */
void HR_heartMonitor_addRedIr(HR_HeartMonitor *heartMonitor, float *array_red,
		float *array_ir, int array_size) {
	if (array_size >= heartMonitor->size) {
		memcpy(heartMonitor->red, &array_red[array_size - heartMonitor->size],
				heartMonitor->size * sizeof(float));
		memcpy(heartMonitor->ir, &array_ir[array_size - heartMonitor->size],
				heartMonitor->size * sizeof(float));
	} else {
		memmove(heartMonitor->red, &heartMonitor->red[array_size],
				(heartMonitor->size - array_size) * sizeof(float));
		memcpy(&heartMonitor->red[heartMonitor->size - array_size], array_red,
				array_size * sizeof(float));

		memmove(heartMonitor->ir, &heartMonitor->ir[array_size],
				(heartMonitor->size - array_size) * sizeof(float));
		memcpy(&heartMonitor->ir[heartMonitor->size - array_size], array_ir,
				array_size * sizeof(float));
	}
}

/**
 * @brief Detects peaks from the green channel data in the HR_HeartMonitor structure.
 *
 * This function detects peaks from the green channel data in the HR_HeartMonitor structure. It
 * applies peak detection and normalization algorithms to the green channel data array.
 *
 * @param heartMonitor Pointer to the HR_HeartMonitor structure.
 * @return None.
 */
void HR_heartMonitor_peaksFromGreen(HR_HeartMonitor *heartMonitor) {
	peaks_detect(heartMonitor->green, heartMonitor->peaks, heartMonitor->size,
			heartMonitor->threshold);
	peaks_normalize(heartMonitor->green, heartMonitor->peaks,
			heartMonitor->size);
}

/**
 * @brief Calculates the heart rate from the detected peaks in the HR_HeartMonitor structure.
 *
 * This function calculates the heart rate from the detected peaks in the HR_HeartMonitor structure.
 * It indexes the peaks, calculates the heart rate using the indexed peaks, and returns the result.
 *
 * @param heartMonitor Pointer to the HR_HeartMonitor structure.
 * @return Heart rate calculated from the detected peaks.
 */
float HR_heartMonitor_heartRateFromPeaks(HR_HeartMonitor *heartMonitor) {
	int indexed_size = 0;
	int *signals_indexed = peaks_indexingSize(heartMonitor->peaks,
			heartMonitor->size, &indexed_size);

	float result = HR_heartMonitor_heartRateFromIndexedPeaks(heartMonitor,
			signals_indexed, indexed_size);

	free(signals_indexed);
	return result;
}

/**
 * @brief Calculates the heart rate from the indexed peaks in the HR_HeartMonitor structure.
 *
 * This function calculates the heart rate from the indexed peaks in the HR_HeartMonitor structure.
 * It iterates through the indexed peaks, calculates the average peak-to-peak distance, and returns
 * the heart rate based on the average distance and the sampling frequency.
 *
 * @param heartMonitor Pointer to the HR_HeartMonitor structure.
 * @param peaks        Pointer to the array of indexed peaks.
 * @param peaks_size   Size of the array of indexed peaks.
 * @return Heart rate calculated from the indexed peaks.
 */
float HR_heartMonitor_heartRateFromIndexedPeaks(HR_HeartMonitor *heartMonitor,
		int *peaks, int peaks_size) {
	int pointer = 0;
	int count = 0;
	int sum = 0.0;
	while (pointer < peaks_size - 1) {
		sum += (peaks[pointer + 1] - peaks[pointer]);
		count++;
		pointer++;
	}

	if (count == 0) {
		return nanf("");
	} else {
		return 60.0 / ((sum / count) / heartMonitor->freq);
	}
}

/**
 * @brief Calculates the ratio from the indexed peaks in the HR_HeartMonitor structure.
 *
 * This function calculates the ratio from the indexed peaks in the HR_HeartMonitor structure.
 * It calculates the AC (alternating current) and DC (direct current) components for the red and IR signals,
 * based on the minimum and maximum values within the specified peaks. It then calculates the ratio
 * between the AC and DC components of the red and IR signals and returns the result.
 *
 * @param heartMonitor Pointer to the HR_HeartMonitor structure.
 * @param peaks        Pointer to the array of indexed peaks.
 * @param peaks_size   Size of the array of indexed peaks.
 * @return Ratio calculated from the indexed peaks.
 */
float HR_heartMonitor_ratioFromIndexedPeaks(HR_HeartMonitor *heartMonitor,
		int *peaks, int peaks_size) {
	if (peaks_size < 2) {
		return nanf("");
	}
	int *mins = (int*) malloc((peaks_size - 1) * sizeof(int));
	int *maxs = (int*) malloc((peaks_size - 2) * sizeof(int));

	// MA_process(heartMonitor->ma_red, heartMonitor->red, heartMonitor->size);
	// MA_process(heartMonitor->ma_ir, heartMonitor->ir, heartMonitor->size);

	// RED
	// find mins
	for (int i = 0; i < peaks_size - 1; i++) {
		mins[i] = findMinIndex(heartMonitor->red, peaks[i], peaks[i + 1]);
	}

	// find maxs between mins
	for (int i = 0; i < peaks_size - 2; i++) {
		maxs[i] = findMaxIndex(heartMonitor->red, mins[i], mins[i + 1]);

	}
	// calculate ratio
	float AC_red = 0.0;
	float DC_red = 0.0;
	int count = 0;
	for (int i = 0; i < peaks_size - 2; i++) {
		count++;
		float DC_now = (heartMonitor->red[mins[i]]
				+ heartMonitor->red[mins[i + 1]]) / 2;
		AC_red += (heartMonitor->red[maxs[i]] - DC_now);
		DC_red += DC_now;
	}

	AC_red = AC_red / count;
	DC_red = DC_red / count;

	// IR
	for (int i = 0; i < peaks_size - 1; i++) {
		mins[i] = findMinIndex(heartMonitor->ir, peaks[i], peaks[i + 1]);
	}
	// find maxs between mins
	for (int i = 0; i < peaks_size - 2; i++) {
		maxs[i] = findMaxIndex(heartMonitor->ir, mins[i], mins[i + 1]);
	}
	// calculate ratio
	float AC_ir = 0.0;
	float DC_ir = 0.0;
	count = 0;
	for (int i = 0; i < peaks_size - 2; i++) {
		count++;
		float DC_now = (heartMonitor->ir[mins[i]]
				+ heartMonitor->ir[mins[i + 1]]) / 2;
		AC_ir += heartMonitor->ir[maxs[i]] - DC_now;
		DC_ir += DC_now;
	}
	AC_ir = AC_ir / count;
	DC_ir = DC_ir / count;

	free(maxs);
	free(mins);
	return (AC_red / DC_red) / (AC_ir / DC_ir);
}

/**
 * @brief Calculates the ratio from the peaks in the HR_HeartMonitor structure.
 *
 * This function calculates the ratio from the peaks in the HR_HeartMonitor structure.
 * It first indexes the peaks and determines the size of the indexed peaks array.
 * Then, it calls the HR_heartMonitor_ratioFromIndexedPeaks function to calculate
 * the ratio based on the indexed peaks. Finally, it frees the memory allocated for the
 * indexed peaks array and returns the calculated ratio.
 *
 * @param heartMonitor Pointer to the HR_HeartMonitor structure.
 * @return Ratio calculated from the peaks.
 */
float HR_heartMonitor_ratioFromPeaks(HR_HeartMonitor *heartMonitor) {

	int peaks_size = 0;
	int *peaks = peaks_indexingSize(heartMonitor->peaks, heartMonitor->size,
			&peaks_size);
	float result = HR_heartMonitor_ratioFromIndexedPeaks(heartMonitor, peaks,
			peaks_size);
	free(peaks);
	return result;
}

/**
 * @brief Calculates the heart rate and ratio from the HR_HeartMonitor structure.
 *
 * This function calculates the heart rate and ratio from the provided HR_HeartMonitor structure.
 *
 * @param heartMonitor Pointer to the HR_HeartMonitor structure.
 * @param rate Pointer to a float variable to store the calculated heart rate.
 * @param ratio Pointer to a float variable to store the calculated ratio.
 *
 * @see HR_heartMonitor_heartRateFromIndexedPeaks
 * @see HR_heartMonitor_ratioFromIndexedPeaks
 */
void HR_heartMonitor_heartRateAndRation(HR_HeartMonitor *heartMonitor,
		float *rate, float *ratio) {
	int peaks_size = 0;
	int *peaks = peaks_indexingSize(heartMonitor->peaks, heartMonitor->size,
			&peaks_size);
	*rate = HR_heartMonitor_heartRateFromIndexedPeaks(heartMonitor, peaks,
			peaks_size);
	*ratio = HR_heartMonitor_ratioFromIndexedPeaks(heartMonitor, peaks,
			peaks_size);
}

/**
 * @brief Normalizes the peaks in the array.
 *
 * This function normalizes the peaks in the given array based on the provided signals.
 *
 * @param array Pointer to the array of float values.
 * @param signals Pointer to the array of signals representing the peaks.
 * @param array_size The size of the array.
 *
 * @note The 'array' and 'signals' arrays should have a size of at least 'array_size'.
 *
 */
void peaks_normalize(float *array, int *signals, int array_size) {
	int start = 0;
	for (int i = 1; i < array_size; i++) {
		if (signals[i] == 1 && signals[i - 1] == 0) {
			start = i;
		}
		if ((signals[i] == 0 && signals[i - 1] == 1) || i == array_size - 1) {

			float max = array[start];
			for (int j = start + 1; j < i; j++) {
				if (max < array[j]) {
					max = array[j];
				}
			}

			int peaks = 0;
			int count = 0;
			for (int j = start; j < i; j++) {
				signals[j] = 0;
				if (array[j] >= max) {
					peaks += j;
					count++;
				}
			}
			if (count != 0) {
				signals[peaks / count] = 1;
			}
		}
	}
}

/**
 * @brief Detects peaks in the array based on a threshold.
 *
 * This function detects peaks in the given array by comparing each element to a threshold value.
 *
 * @param array Pointer to the array of float values.
 * @param signals Pointer to the array of signals representing the peaks.
 * @param array_size The size of the array.
 * @param initial_threshold The initial threshold value for peak detection.
 *
 * @note The 'array' and 'signals' arrays should have a size of at least 'array_size'.
 *
 */
void peaks_detect(float *array, int *signals, int array_size,
		float initial_threshold) {
	float threshold = array[0];
	for (int i = 1; i < array_size; i++) {
		if (array[i] > threshold) {
			threshold = array[i];
		}
	}
	threshold = initial_threshold * threshold;

	for (int i = 0; i < array_size; i++) {
		if (array[i] > threshold) {
			signals[i] = 1;
		} else {
			signals[i] = 0;
		}
	}
}

/**
 * @brief Creates an indexed array of peak positions.
 *
 * This function takes an array of signals representing peaks and creates an indexed array
 * containing the positions of the peaks.
 *
 * @param signals Pointer to the array of signals representing the peaks.
 * @param ret_signals Pointer to the array where the indexed positions will be stored.
 * @param array_size The size of the signals array.
 *
 * @note The 'signals' and 'ret_signals' arrays should have a size of at least 'array_size'.
 *
 */
void peaks_indexing(int *signals, int *ret_signals, int array_size) {
	int pointer = 0;
	for (int i = 0; i < array_size; i++) {
		if (signals[i] == 1) {
			ret_signals[pointer] = i;
			pointer++;
		}
	}
	ret_signals[pointer] = -1;
}

/* @brief Creates an indexed array of peak positions and returns the size.
 *
 * This function takes an array of signals representing peaks and creates an indexed array
 * containing the positions of the peaks. It also returns the size of the indexed array.
 *
 * @param signals Pointer to the array of signals representing the peaks.
 * @param array_size The size of the signals array.
 * @param result_size Pointer to an integer where the size of the indexed array will be stored.
 *
 * @return Pointer to the indexed array of peak positions.
 *
 * @note The returned indexed array should be freed using the 'free' function after use.
 *
 */
int* peaks_indexingSize(int *signals, int array_size, int *result_size) {
	int count = 0;
	for (int i = 0; i < array_size; i++) {
		if (signals[i] == 1) {
			count++;
		}
	}
	*result_size = count;
	int *result = (int*) malloc(count * sizeof(int));
	int pointer = 0;
	for (int i = 0; i < array_size; i++) {
		if (signals[i] == 1) {
			result[pointer] = i;
			pointer++;
		}
	}

	return result;
}

/**
 * @brief Creates a new Slope Sum Function (SSF) object.
 *
 * This function allocates memory for a new SSF object and initializes its properties.
 *
 * @param window_size The size of the SSF window.
 *
 * @return Pointer to the newly created SSF object.
 *
 * @note The returned SSF object should be freed using the 'SSF_free' function after use.
 *
 */
SSF* SSF_new(int window_size) {
	SSF *ssf = (SSF*) malloc(sizeof(SSF));
	ssf->window_size = window_size;
	ssf->data = (float*) malloc(window_size * sizeof(float));
	return ssf;
}

/**
 * @brief Frees the memory allocated for a Slope Sum Function (SSF) object.
 *
 * This function frees the memory allocated for the SSF object, including its data array.
 *
 * @param ssf Pointer to the SSF object to be freed.
 *
 */
void SSF_free(SSF *ssf) {
	free(ssf->data);
	free(ssf);
}

/**
 * @brief Calculates the Slope Sum Function (SSF) of an input array.
 *
 * This function calculates the SSF of an input array by sliding a window of size
 * ssf->window_size over the array. For each position of the window, it calculates
 * the sum of positive differences between adjacent elements in the window.
 * The resulting SSF values replace the corresponding elements in the input array.
 *
 * @param ssf Pointer to the SSF object.
 * @param array Pointer to the input array.
 * @param array_size Size of the input array.
 *
 */
void SSF_process(SSF *ssf, float *array, int array_size) {
	for (int i = 0; i < array_size; i++) {
		for (int j = 1; j < ssf->window_size; j++) {
			ssf->data[j] = ssf->data[j - 1];
		}
		ssf->data[0] = array[i];
		float sum = 0.0;
		for (int j = 0; j < ssf->window_size - 1; j++) {
			float delta = ssf->data[j] - ssf->data[j + 1];
			if (delta >= 0) {
				sum += delta;
			}
		}
		array[i] = sum;
	}
}

/**
 * @brief Creates a new Moving Average (MA) filter object.
 *
 * This function creates a new MA_filter object with the specified window size.
 * The MA filter is a sliding window filter that calculates the average of the
 * windowed samples. It is commonly used for smoothing or noise reduction.
 *
 * @param window_size The size of the moving average window.
 * @return Pointer to the newly created MA_filter object.
 *
 */
MA_filter* MA_new(int window_size) {
	MA_filter *filter = malloc(sizeof(MA_filter));
	filter->window_size = window_size;
	filter->data = calloc(window_size - 1, sizeof(float));
	return filter;

}

/**
 * @brief Frees the memory allocated for a Moving Average (MA) filter object.
 *
 * This function frees the memory allocated for the MA_filter object, including
 * the data array and the filter object itself.
 *
 * @param filter Pointer to the MA_filter object to be freed.
 *
 */
void MA_free(MA_filter *filter) {
	free(filter->data);
	free(filter);
}

/**
 * @brief Applies the Moving Average (MA) filter to an array of values.
 *
 * This function applies the Moving Average (MA) filter to an array of values.
 * It updates the data array of the MA_filter object with the new values from
 * the input array and computes the moving average for each element in the array.
 *
 * @param filter Pointer to the MA_filter object.
 * @param array  Pointer to the array of values to be filtered.
 * @param array_size The size of the array.
 *
 */
void MA_process(MA_filter *filter, float *array, int array_size) {
	for (int i = 0; i < array_size; i++) {
		for (int j = filter->window_size - 1; j > 0; j--) {
			filter->data[j] = filter->data[j - 1];
		}
		filter->data[0] = array[i];
		float sum = 0.0;
		for (int j = 0; j < filter->window_size; j++) {
			sum += filter->data[j];
		}
		array[i] = sum / filter->window_size;
	}
}

/**
 * @brief Creates a new Bandpass Filter (BPF) object.
 *
 * This function creates a new Bandpass Filter (BPF) object with the specified
 * parameters. The BPF object stores the filter order, frequency, and frequency
 * range for the bandpass filter. It also computes the coefficients and
 * intermediate values required for the filter.
 *
 * @param order The order of the bandpass filter.
 * @param freq The sampling frequency.
 * @param low The lower cutoff frequency.
 * @param high The higher cutoff frequency.
 *
 * @return Pointer to the newly created BPF_filter object.
 */
BPF_filter* BPF_new(int order, float freq, float low, float high) {
	BPF_filter *bpf_filter = (BPF_filter*) malloc(sizeof(BPF_filter));

	bpf_filter->n = order;
	bpf_filter->freq = freq;
	bpf_filter->freq_low = low;
	bpf_filter->freq_high = high;

	float a, b, r;
	a = cosf(M_PI * (high + low) / freq) / cosf(M_PI * (high - low) / freq);
	b = tanf(M_PI * (high - low) / freq);

	bpf_filter->n = bpf_filter->n / 2;
	bpf_filter->A = (float*) malloc(bpf_filter->n * sizeof(float));
	bpf_filter->d1 = (float*) malloc(bpf_filter->n * sizeof(float));
	bpf_filter->d2 = (float*) malloc(bpf_filter->n * sizeof(float));
	bpf_filter->d3 = (float*) malloc(bpf_filter->n * sizeof(float));
	bpf_filter->d4 = (float*) malloc(bpf_filter->n * sizeof(float));

	bpf_filter->w0 = (float*) calloc(bpf_filter->n, sizeof(float));
	bpf_filter->w1 = (float*) calloc(bpf_filter->n, sizeof(float));
	bpf_filter->w2 = (float*) calloc(bpf_filter->n, sizeof(float));
	bpf_filter->w3 = (float*) calloc(bpf_filter->n, sizeof(float));
	bpf_filter->w4 = (float*) calloc(bpf_filter->n, sizeof(float));

	for (int i = 0; i < bpf_filter->n; ++i) {
		r = sinf(M_PI * (2.0 * i + 1.0) / (4.0 * bpf_filter->n));
		freq = b * b + 2.0 * b * r + 1.0;
		bpf_filter->A[i] = b * b / freq;
		bpf_filter->d1[i] = 4.0 * a * (1.0 + b * r) / freq;
		bpf_filter->d2[i] = 2.0 * (b * b - 2.0 * a * a - 1.0) / freq;
		bpf_filter->d3[i] = 4.0 * a * (1.0 - b * r) / freq;
		bpf_filter->d4[i] = -(b * b - 2.0 * b * r + 1.0) / freq;
	}

	return bpf_filter;
}

/**
 * @brief Frees the memory allocated for a Bandpass Filter (BPF) object.
 *
 * This function frees the memory allocated for a Bandpass Filter (BPF) object.
 * It releases the memory occupied by the filter coefficients and intermediate
 * values, as well as the BPF object itself.
 *
 * @param bpf_filter Pointer to the BPF_filter object to be freed.
 */
void BPF_free(BPF_filter *bpf_filter) {
	free(bpf_filter->A);
	free(bpf_filter->d1);
	free(bpf_filter->d2);
	free(bpf_filter->d3);
	free(bpf_filter->d4);
	free(bpf_filter->w0);
	free(bpf_filter->w1);
	free(bpf_filter->w2);
	free(bpf_filter->w3);
	free(bpf_filter->w4);
	free(bpf_filter);

}

/**
 * @brief Applies Bandpass Filtering to an input array.
 *
 * This function applies Bandpass Filtering to an input array using the given
 * Bandpass Filter (BPF) object. It iterates through the input array and updates
 * each element with the filtered value based on the BPF coefficients and
 * intermediate values.
 *
 * @param filter Pointer to the BPF_filter object.
 * @param array Pointer to the input array to be filtered.
 * @param array_size The size of the input array.
 */
void BPF_process(BPF_filter *filter, float *array, int array_size) {
	for (int i = 0; i < array_size; i++) {
		for (int j = 0; j < filter->n; ++j) {
			filter->w0[j] = filter->d1[j] * filter->w1[j]
					+ filter->d2[j] * filter->w2[j]
					+ filter->d3[j] * filter->w3[j]
					+ filter->d4[j] * filter->w4[j] + array[i];
			array[i] = filter->A[j]
					* (filter->w0[j] - 2.0 * filter->w2[j] + filter->w4[j]);
			filter->w4[j] = filter->w3[j];
			filter->w3[j] = filter->w2[j];
			filter->w2[j] = filter->w1[j];
			filter->w1[j] = filter->w0[j];
		}
	}
}

/**
 * @brief Finds the index of the maximum value in a given range of an array.
 *
 * This function searches for the index of the maximum value within a specified
 * range of an input array. It starts searching from the 'start' index up to the
 * 'end' index (exclusive), and returns the index of the maximum value found.
 * If multiple elements have the same maximum value, the function returns the
 * index of the first occurrence.
 *
 * @param array Pointer to the input array.
 * @param start The starting index of the range (inclusive).
 * @param end The ending index of the range (exclusive).
 * @return The index of the maximum value within the specified range.
 */
int findMaxIndex(float *array, int start, int end) {
	int index = start;
	for (int i = start; i < end; i++) {
		if (array[i] > array[index]) {
			index = i;
		}
	}
	return index;
}

/**
 * @brief Finds the index of the minimum value in a given range of an array.
 *
 * This function searches for the index of the minimum value within a specified
 * range of an input array. It starts searching from the 'start' index up to the
 * 'end' index (exclusive), and returns the index of the minimum value found.
 * If multiple elements have the same minimum value, the function returns the
 * index of the first occurrence.
 *
 * @param array Pointer to the input array.
 * @param start The starting index of the range (inclusive).
 * @param end The ending index of the range (exclusive).
 * @return The index of the minimum value within the specified range.
 */
int findMinIndex(float *array, int start, int end) {
	int index = start;
	for (int i = start; i < end; i++) {
		if (array[i] < array[index]) {
			index = i;
		}
	}
	return index;
}

