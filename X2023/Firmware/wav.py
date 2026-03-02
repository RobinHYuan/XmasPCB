import wave
import numpy

# Read file to get buffer                                                                                               
ifile = wave.open("test.wav")
samples = ifile.getnframes()
audio = ifile.readframes(samples)

# Convert buffer to uint8 array
audio_as_np_uint8 = numpy.frombuffer(audio, dtype=numpy.uint8)

numpy.savetxt("ach.csv", audio_as_np_uint8, delimiter=",")