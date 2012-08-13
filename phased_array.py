import scipy
import numpy as np
import struct
import serial

def serialopen(port, baudrate=9600, stopbits=1):
	ser = serial.Serial(port)
	ser.baudrate = baudrate
	ser.stopbits = stopbits
	return ser

def synchronize(ser,chunksize=8960,wordlength=2):
	while True:
		temp = ser.read(wordlength)
		if int(byte2int(temp,wordlength=wordlength)[0]) >> 12 == 1:
			ser.read(chunksize-wordlength)
			return ser



def serialread(ser,chunksize=8960, *args, **kwargs):
	while ser.isOpen():
		yield byte2int(ser.read(chunksize),*args,**kwargs)
	print 'Serial Connection Closed'

# Iterator to read a chunksize worth of bytes from a binary file
def fileread(fn, chunksize=8960,*args,**kwargs):
	with open(fn,'rb') as f:
		while True:
			chunk = f.read(chunksize)
			if chunk:
				yield byte2int(chunk,*args,**kwargs)
			else:
				break

# returns the binary to int
def byte2int(chunks,wordlength=2, *args, **kwargs):
	n = len(chunks)/wordlength
	frame = np.zeros(n)
	for i,ind in enumerate(range(0,len(chunks),wordlength)):
		frame[i] = struct.unpack('h',chunks[ind:ind+wordlength])[0]
	return frame

def remove_dc_offset(frame):
	mean = frame.sum(1) / frame.shape[1]
	for f,m in zip(frame,mean):
		f -= m
	return frame

def toiq(frame,nfft=2048):
	win = scipy.hamming(frame.shape[1])
	win.shape = (1,len(win))
	win = win.repeat(frame.shape[0],axis=0)
	data = scipy.ifft(win*frame,nfft,axis=1)
	return data[:,0:nfft/2]	

def process_file(fn,mti=False,rti=True,*args,**kwargs):
	numElements = 14
	numSamples = 320

	if rti:
		import matplotlib.pyplot as plt
		fig = plt.figure()
		rtiBuf = np.zeros((512,1024))

	lastFrame = None
	for rawData in fileread(fn):
		rawData.shape = (numElements,numSamples)
		frame = remove_dc_offset(rawData)
		
		if mti:
			if lastFrame is None: 
				lastFrame = frame
				continue
			data = frame - lastFrame
			lastFrame = frame
		else:
			data = frame		
		
		rci = toiq(data)
		
		if rti:
			rcs = np.sqrt( rci.real**2 + rci.imag**2)
			val = rcs.sum(0) / 14
			rtiBuf[0:511,:] = rtiBuf[1:512,:]
			rtiBuf[511,:] = 20*np.log10(val)	
		
			if fig is not None:
				fig.canvas.mpl_connect('pick_event',fig_click)
			plt.clf()
			plt.imshow(rtiBuf,aspect=2,vmin=-40,vmax=10,norm=None, extent=(0,1023,0,511))
			plt.draw()

def fig_click(data,mouseevent):
	print mouseevent	

def main():
    fn = 'data_20120809_intersection.bin'
    chunks = fileread_frame(fn)
    bytes = next(chunks)
    frame = byte2int(bytes)
    return (frame)
