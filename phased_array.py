#!/usr/bin/env python

from scipy import hamming, fftpack
import numpy as np
import struct
import serial
import matplotlib.pyplot as plt

def serialopen(port, baudrate=9600, stopbits=1, *args, **kwargs):
	ser = serial.Serial(port)
	ser.baudrate = baudrate
	ser.stopbits = stopbits
	return ser

def synchronize(ser,chunksize=8960,wordlength=2, *args, **kwargs):
	while True:
		temp = ser.read(wordlength)
		if int(struct.unpack('h',temp)[0]) >> 12 == 1:
			ser.read(chunksize-wordlength)
			return ser

def serialread(ser,chunksize=8960, wordlength=2, shape=None, *args, **kwargs):
	while True:
		if ser.isOpen() and ser.readable():
			data = [struct.unpack('h',c) for c in chunks(ser.read(chunksize),wordlength)]
			if shape is not None:
				data = np.array(data)
				data.shape = shape
			yield data & 2**12-1
		else:
			break

def fileread(fn, chunksize=8960, wordlength=2, shape=None,*args,**kwargs):
	"""
		Yeilds frames of data from a binary file of given chunksize
	"""
	with open(fn,'rb') as f:
		while True:
			bytes = f.read(chunksize)
			if bytes:
				data = [struct.unpack('h',c) for c in chunks(bytes,wordlength)]
				if shape is not None:
					data = np.array(data)
					data.shape = shape
				yield data
			else:
				break

def chunks(l, n):
    """ 
		Yield successive n-sized chunks from l.
    """
    for i in xrange(0, len(l), n):
        yield l[i:i+n]

def remove_dc_offset(frame):
	"""
	Removes DC offset from ADC output
	"""
	mean = frame.sum(1) / frame.shape[1]
	for f,m in zip(frame,mean):
		f -= m
	return frame

def toiq(frame,nfft=2048):
	"""
	Converts ADC output to range in-phase (I) and quadrature (Q) data, i.e., complex range profile
	"""
	win = hamming(frame.shape[1])
	win.shape = (1,len(win))
	win = win.repeat(frame.shape[0],axis=0)
	data = fftpack.ifft(win*frame,nfft,axis=1)
	return data[:,0:nfft/2]	

def process(port=None,file=None,numElements=14, numSamples=320, mti=True,plot=None,*args,**kwargs):
	"""
	Process data from a file or a serial port
	"""
	if port is not None:
		ser = serialopen(port, *args, **kwargs)
		ser = synchronize(ser, *args, **kwargs)
		frames = serialread(ser,shape=(numElements,numSamples), *args, **kwargs)
	elif file is not None:
		frames = fileread(file,shape=(numElements,numSamples), *args, **kwargs)	

	lastFrame = None
	for rawData in frames:
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
		
		if plot is not None:
			plot.draw(rci)
		
class RTI:
	def __init__(self, bufferSize=512, rangeGates=1024):
		self.fig = plt.figure()
		self.rtiBuf = np.zeros((bufferSize,rangeGates))
		self.ny = bufferSize
		self.nx = rangeGates
			
	def draw(self,rci):
			rcs = np.sqrt( rci.real**2 + rci.imag**2)
			val = rcs.sum(0) / 14
			self.rtiBuf[0:511,:] = self.rtiBuf[1:512,:]
			self.rtiBuf[511,:] = 20*np.log10(val)	
		
			plt.figure(self.fig.number)
			plt.clf()	
			plt.imshow(self.rtiBuf,aspect=2,vmin=-40,vmax=10,norm=None, extent=(0,self.nx-1,0,self.ny-1))
			plt.draw()

def main():
	from optparse import OptionParser

	parser = OptionParser()
	parser.add_option("-f", "--file", dest="filename", help="binary file to read from", metavar="FILE")
	parser.add_option("-p", "--port", dest="portname", help="serial port to read from", metavar="PORT")
	parser.add_option("-m", "--mti", action="store_true", dest="mti", default=False, help="Turn on two pulse cancelor")

	(options, args) = parser.parse_args()
	if len(args) == 1:
		parser.error("incorrect number of arguments")

	if options.portname and options.filename:
		parser.error('options port and file are mutually exclusive')

	if options.portname:
		try:
			process(port=options.portname,mti=options.mti,plot=RTI())
		except KeyboardInterrupt:
			raise
	elif options.filename:
		try:
			process(file=options.filename,mti=options.mti,plot=RTI())
		except KeyboardInterrupt:
			raise
if __name__ == '__main__':
	main()
