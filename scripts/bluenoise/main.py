from pathlib import Path

import numpy as np
from numpy import fft

N = 128
SLICES = 64
ITERATIONS = 4

fx = fft.fftfreq(N)
fy = fft.fftfreq(N)
x, y = np.meshgrid(fx, fy)
sigma = 0.2
H = 1 - np.exp(-(x*x + y*y) / (2 * sigma**2))

tex = np.zeros((SLICES, N, N), dtype=np.uint16)
for i in range(SLICES):
    I = np.random.random((N, N))

    for _ in range(ITERATIONS):
        F = fft.fft2(I)
        F *= H
        I = fft.ifft2(F).real

        order = np.argsort(I.ravel())
        ranks = np.empty(N * N, dtype=np.int64)
        ranks[order] = np.arange(N * N)
        I = (ranks.reshape(N, N) + 0.5) / (N * N)

    tex[i] = np.floor(I * 65536)

tex.tofile(Path(__file__).resolve().parents[2] / 'assets' / 'noise' / 'blue_noise.bin')

