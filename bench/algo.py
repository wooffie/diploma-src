import numpy as np
from spicy import signal
from collections import deque


def bandpass(array, freq=50.0, low=0.5, high=5.0, order=4):
    nyq = freq * 0.5
    lowcut = low / nyq
    highcut = high / nyq

    b, a = signal.butter(order, [lowcut, highcut], btype='bandpass')
    filtered = signal.filtfilt(b, a, array)
    return filtered


def ssf(array, window):
    windows = deque(maxlen=window)
    result = [0.0] * len(array)
    for i in range(1, len(array)):
        if array[i] > array[i - 1]:
            windows.append(array[i] - array[i - 1])
        else:
            windows.append(0)
        result[i] = sum(windows)
    return result


def ma(array, window):
    windows = deque(maxlen=window)
    result = [0.0] * len(array)
    for i in range(1, len(array)):
        windows.append(array[i])
        result[i] = sum(windows) / len(windows)
    return result


def find_peaks(array, threshold):
    limit = max(array) * threshold
    result = np.array(array) > limit
    result = result.astype(int)
    return normalize_peaks(array, result)


def normalize_peaks(data, peaks):
    normalized_peaks = np.zeros_like(peaks)
    indices = np.where(peaks)[0]

    if len(indices) > 0:
        start = indices[0]
        for i in range(1, len(indices)):
            if indices[i] != indices[i - 1] + 1:
                end = indices[i - 1]
                highest_peak_index = start + np.argmax(data[start:end + 1])
                normalized_peaks[highest_peak_index] = True
                start = indices[i]

        end = indices[-1]
        highest_peak_index = start + np.argmax(data[start:end + 1])
        normalized_peaks[highest_peak_index] = True

    return normalized_peaks


def average_periods(peaks):
    indices = np.where(peaks)[0]
    periods = []
    for i in range(1, len(indices)):
        periods.append(indices[i] - indices[i - 1])
    if len(periods) == 0:
        return 0
    else:
        return sum(periods) / len(periods)
