import numpy as np
import matplotlib.pyplot as plt
import sys


# a = np.array([-32767]*1024)
# b = np.array([32767]*1024)
# c=np.concatenate((a,b))
# np.savetxt("sqr.txt",[c],delimiter=',')
# np.savetxt("sqr.txt",[c],delimiter=',',fmt='%d')


# Original samples (example: sine wave with noise)
wave_in = np.array([0.0, 0.5, 0.7, 0.5, 0.0, -0.5, -0.7, -0.5])

file = 'korg/HN613256P_T70.w0'
if len(sys.argv) > 1:
  file = sys.argv[1]

print("using", file)

int_array = np.fromfile(file, dtype=np.int16, sep=',')

expand = 64

if len(sys.argv) > 2:
  expand = int(sys.argv[2])

print("has", len(int_array), "samples")
print("expand to", len(int_array) * expand)

float_array = int_array.astype(np.float32) / np.iinfo(np.int16).max
float_array[int_array == np.iinfo(np.int16).min] = -1.0

wave_in = float_array

# Perform FFT
fft_result = np.fft.fft(wave_in)


# Zero-pad in the frequency domain to increase resolution
original_length = len(wave_in)
padding_length = original_length * expand  # increase the resolution
fft_padded = np.zeros(padding_length, dtype=complex)
fft_padded[:original_length//expand] = fft_result[:original_length//expand]
fft_padded[-original_length//expand:] = fft_result[-original_length//expand:]

# Perform Inverse FFT
wave_new = np.fft.ifft(fft_padded).real


# Normalize to match the original amplitude
wave_new = wave_new * (original_length / padding_length)

min_val = np.min(wave_new)
max_val = np.max(wave_new)

wave_max = 2 * (wave_new - min_val) / (max_val - min_val) - 1


# Plot the results
plt.figure(figsize=(10, 6))
plt.plot(wave_in, label="Original Samples") #, marker='o')
plt.plot(np.linspace(0, len(wave_in) - 1, padding_length), wave_max, label="High-Resolution Samples", linestyle='--') #, marker='.')
plt.legend()
plt.title("FFT-based High-Resolution Waveform Reconstruction")
plt.xlabel("Sample Index")
plt.ylabel("Amplitude")
plt.grid()
plt.show()

wave_out = np.round(wave_max * 32767).astype(np.int16)
np.savetxt("out.list", [wave_out], delimiter=',', fmt='%d')
