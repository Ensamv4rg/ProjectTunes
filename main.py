import numpy as np
import pandas as pd
import sounddevice as sd

f = open("practice1.wav", "rb")

data = f.read()

#in the raw wav file. Bytes in position 5-8 store the size of the file.
size = (data[4]) | (data[5] << 8) | (data[6] << 16) | (data[7] << 24)
format_tag = data[20] | (data[21] << 8)
channel_size = (data[22] | data[23]<<8)
sample_rate = (data[24] | (data[25] <<8) | (data[26] <<16) | (data[27] <<24))
byte_rate = (data[28] | (data[29] <<8)| (data[30] <<16)| (data[31] <<24))
sample_size = (data[32] | data[33]<<8)
bits_per_sample = (data[34] | data[35] <<8)

data_starts = data.find(b'data')
data_size = (data[data_starts + 4] | (data[data_starts + 5]<<8) | (data[data_starts + 6]<<16) | (data[data_starts + 7]<<24))


print(f"File Size: {size/1048576} megabytes")
print(f"Format Tag: {format_tag}")
print(f"{channel_size} channels")
print(f"Sample Rate: {sample_rate}hz")
print(f"Byte Rate: {byte_rate}")
print(f"Sample Size: {sample_size}")
print(f"Bits per sample: {bits_per_sample}")
print(f"Data Size is {data_size} bytes")

if format_tag == 1:  # PCM
    if bits_per_sample == 8:
        audio_map = np.uint8
    elif bits_per_sample == 16:
        audio_map = np.int16
    elif bits_per_sample == 24:
        # Handle 24-bit, e.g., read as bytes and process
        pass
    elif bits_per_sample == 32:
        audio_map = np.int32
    else:
        raise ValueError("Unsupported bits per sample for PCM")
elif format_tag == 3:  # IEEE float
    if bits_per_sample == 32:
        audio_map = np.float32
    elif bits_per_sample == 64:
        audio_map = np.float64
    else:
        raise ValueError("Unsupported bits per sample for float")
else:
    raise ValueError("Unsupported format tag")

audio_data = np.frombuffer(data[data_starts+8:data_starts+8+data_size], dtype=audio_map)

rows = int(audio_data.shape[0]/2)

audio_data.resize(rows,channel_size)
print(audio_data.shape)

sd.play(audio_data, samplerate=sample_rate)
sd.wait()

df = pd.DataFrame(audio_data, columns=['left','right'])
    





