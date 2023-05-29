import numpy as np
import scipy


def normalize(array, low, high):
    return (array - min(array)) / (max(array) - min(array)) * (high - low) + low


def extract_mat_gap(filename, freq=50):
    data = scipy.io.loadmat(filename)
    pgg = data['sigPPG']
    pgg0 = pgg[0].astype(float)
    pgg1 = pgg[1].astype(float)
    pgg2 = pgg[2].astype(float)
    ticks = np.arange(len(pgg0))
    time = ticks / freq

    timeECG = data['timeECG'].ravel()
    bpmECG = data['bpmECG'].ravel()
    bpm_interp = np.interp(time, timeECG, bpmECG)

    result = {'PGG': [pgg0, pgg1, pgg2], 'Ticks': ticks, 'Time': time, 'BPM': bpm_interp}
    return result
