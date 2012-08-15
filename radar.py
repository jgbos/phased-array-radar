# number of "virtual" elements
numElements=14

# number of samples from ADC
numSamples=320

# FFT Size for ADC to IQ
NFFT = 2048

# FFT Size for Range-Angle
NFFT_ANGLE = 128

# (m) 2.5/2 inch phase center spacing
delta_x = 2.5*(1/12.)*0.3048    

# (Hz) LFM start frequency
fStart = 2400e6                

# (Hz) LFM stop frequency
fStop = 2480e6                 

# (Hz) carrier frequency
fc = (fStart + fStop)/2        

# (Hz) transmit bandwidth
BW = fStop - fStart            

# (m) range resolution
delta_r = 3e8 / (2*BW)