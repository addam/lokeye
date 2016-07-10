#!/usr/bin/python3
if 0:
    rows = open("grid_calibration.tab").readlines()
    data = list()
    for i in range(0, len(rows), 4):
        block = " ".join(rows[i:i+4])
        data.append([float(v) for v in block.split()])
    import matplotlib.pyplot as plt
    fig, ax = plt.subplots()
    #for ex, ey, gx, gy, fx, fy in data:
        #dummy = ax.annotate("{} {}".format(gx, gy), (x, y))
    def clamp(val, mean, covar):
        val = (val - mean) / (2 * covar)
        return 1 if val > 1 else 0 if val < 0 else val
    import numpy as np
    arr = np.array(data)
    covar = np.cov(arr, rowvar=False)
    sy = covar[4][4]**0.5
    sx = covar[5][5]**0.5
    mx, my = np.average(arr, 0)[4:6]
    ax.scatter([d[2] for d in data], [d[3] for d in data], c=[(clamp(d[4], mx, sx), clamp(d[5], my, sy), 0.5, 1) for d in data])
    plt.show(ax)

import numpy as np
import matplotlib.pyplot as plt

filename = "grid_calibration_20_fine.tab"
data = [[float(v) for v in r.split()] for r in open(filename).readlines()]
data.sort()
data = [[a + b for (a, b) in zip(data[i], data[i+1])] for i in range(0, len(data), 2)]

fig, ax = plt.subplots(5, 4)
for i, subplot in enumerate((s for row in ax for s in row), 2):
    mat = np.array([d[i] for d in data]).reshape(16, 9).T
    mat -= np.mean(mat)
    mat = 1 / (1 + np.exp(-mat / np.std(mat)))
    #mat = np.minimum(np.maximum(mat, -np.std(mat)), np.std(mat))
    subplot.matshow(mat)

fig.show()
input()
