import numpy as np
import matplotlib.pyplot as plt
import sys
from scipy.fft import fft, ifft

# Original samples (example: sine wave with noise)
original_samples = np.array([0.0, 0.5, 0.7, 0.5, 0.0, -0.5, -0.7, -0.5])

file = 'korg/HN613256P_T70.w0'
if len(sys.argv) > 1:
  file = sys.argv[1]
print("using", file)
int_array = np.fromfile(file, dtype=np.int16, sep=',')
float_array = int_array.astype(np.float32) / np.iinfo(np.int16).max
float_array[int_array == np.iinfo(np.int16).min] = -1.0

original_samples = float_array
original_x = np.linspace(0, len(original_samples) - 1, len(original_samples))

# Perform FFT and zero-pad for higher resolution
N = len(original_samples)
high_res_N = N * 10  # Increase resolution by a factor of 10
fft_result = fft(original_samples)
padded_fft = np.zeros(high_res_N, dtype=complex)
padded_fft[:N // 2] = fft_result[:N // 2]
padded_fft[-(N // 2):] = fft_result[-(N // 2):]

# Perform inverse FFT to get high-resolution data
high_res_samples = ifft(padded_fft).real

# Generate high-resolution x-axis
high_res_x = np.linspace(0, len(original_samples) - 1, high_res_N)

# Plotting
plt.figure(figsize=(10, 5))

# Original samples
plt.plot(original_x, original_samples, 'o-', label='Original Samples')

# High-resolution samples
plt.plot(high_res_x, high_res_samples, '-', label='High-Resolution Samples')

plt.title('Original vs. High-Resolution Samples')
plt.legend()
plt.xlabel('Sample Index')
plt.ylabel('Amplitude')
plt.grid(True)
plt.show()
