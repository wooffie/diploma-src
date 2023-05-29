import matplotlib.pyplot as plt
from algo import bandpass, ssf, find_peaks, average_periods, ma

from utils import extract_mat_gap, normalize

if __name__ == '__main__':
    data = extract_mat_gap('./gyro_acc_ppg/Subject_3.mat')
    freq = 50.0
    size = 200
    batch = 50

    pgg = data['PGG']

    pgg1 = normalize(pgg[0], 0.5, 1.0)
    pgg2 = normalize(pgg[1], 0.5, 1.0)
    pgg3 = normalize(pgg[2], 0.5, 1.0)

    time = data['Time']
    filtered = bandpass(pgg1, freq, 1.0, 8.5, 8)
    s = ssf(filtered, 8)
    p = []
    calc = []
    calc_time = []

    for i in range(0, len(s), batch):
        start_index = i
        end_index = i + size
        arr = s[start_index:end_index]
        peaks = find_peaks(arr, 0.3)
        avg = average_periods(peaks)
        if avg == 0:
            hr = 0
        else:
            hr = 60.0 / (avg / freq)
        calc.append(hr)
        calc_time.append(time[i])
        p[i:i + batch] = (arr * peaks)[0:batch]

    calc = ma(calc, 20)

    plt.title("HR algorithm")
    plt.xlabel("t, sec")
    plt.ylabel("Beats per minute")

    plt.plot(time, data['BPM'], label='ECG bpm')
    plt.plot(calc_time, calc, label='Calculated')
    plt.legend()
    plt.show()

    plt.title("HR algorithm")
    plt.xlabel("t, sec")
    plt.ylabel("Values")

    plt.plot(time, pgg1, label='Original signal')
    plt.plot(time, filtered, label='BP')
    plt.plot(time, s, label='SSF')
    plt.plot(time, p, label='Peaks')

    plt.legend()
    plt.show()
