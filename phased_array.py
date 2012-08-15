#!/usr/bin/env python

from scipy import hamming, fftpack
import numpy as np
import struct
import serial
from plots import DataDraw
import radar

def serialopen(port, baudrate=9600, stopbits=1, *args, **kwargs):
	ser = serial.Serial(port)
	ser.baudrate = baudrate
	ser.stopbits = stopbits
	return ser

def synchronize(ser,chunksize=8960, wordlength=2, *args, **kwargs):
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
			yield remove_dc_offset(data & 2**12-1)
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
				yield remove_dc_offset(data)
			else:
				break

def filewrite(f, data, chunksize=8960, wordlength=2, *args, **kwargs):
	tmp = data.copy()
	tmp.shape = (data.shape[0]*data.shape[1],)
	bytes = [struct.pack('h',c) for c in tmp]
	f.write(''.join(bytes))
		
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

def process(port=None,file=None, mti=True,ptype=None,record=None, *args,**kwargs):
	"""
	Process data from a file or a serial port
	"""
	numElements = radar.numElements
	numSamples = radar.numSamples

	if port is not None:
		ser = serialopen(port, *args, **kwargs)
		ser = synchronize(ser, *args, **kwargs)
		frames = serialread(ser,shape=(numElements,numSamples), *args, **kwargs)
	elif file is not None:
		frames = fileread(file,shape=(numElements,numSamples), *args, **kwargs)	

	plot = DataDraw()
	plot.set_mode(ptype)

	if record is not None:
		rec = open(record,'wb')

	lastFrame = None
	for frame in frames:
		
		if record is not None:
			filewrite(rec,frame)
	
		if plot.mode == 'raw':
			plot.draw(frame)
	
		if mti:
			if lastFrame is None: 
				lastFrame = frame
				continue
			data = frame - lastFrame
			lastFrame = frame
		else:
			data = frame		
	
		if plot.mode == 'raw-mti':
			plot.draw(data)
	
		rci = toiq(data)
	
		if plot.mode == 'rti' or plot.mode=='ati' or plot.mode=='rti-ci':
			plot.draw(rci)

def main():
	from optparse import OptionParser

	parser = OptionParser()
	parser.add_option("-f", "--file", dest="filename", 
										help="binary file to read from", metavar="FILE")
	parser.add_option("-p", "--port", dest="portname", 
										help="serial port to read from", metavar="PORT")
	parser.add_option("-r", "--record", dest="record", 
										help="file to write data to", metavar="FILENAME")
	parser.add_option("--mti", action="store_true", dest="mti", default=False, 
										help="Turn on two pulse cancelor")
	parser.add_option("--raw", action="store_true", dest="raw", default=False, 
										help="plot raw data")
	parser.add_option("--raw-mti", action="store_true", dest="rawmti", default=False, 
										help="plot raw data with two-pulse cancel")
	parser.add_option("--rti-avg", action="store_true", dest="rti", default=False, 
										help="plot RTI averaged across channels")
	parser.add_option("--rti-max", action="store_true", dest="rtici", default=False, 
										help="plot RTI using max over angle bins in range-angle space")
	parser.add_option("--range-angle", action="store_true", dest="angle", default=False, 
										help="plot the range-angle data")

	(options, args) = parser.parse_args()
	if len(args) == 1:
		parser.error("incorrect number of arguments")

	if options.portname and options.filename:
		parser.error('options port and file are mutually exclusive')

	plot = 'rti'
	if options.raw:
		plot = 'raw'
	elif options.rawmti:
		plot = 'raw-mti'
	elif options.angle:
		plot = 'ati'
	elif options.rtici:
		plot = 'rti-ci'

	rec = None
	if options.record:
		rec = options.record

	if options.portname:
		try:
			process(port=options.portname,mti=options.mti,ptype=plot,record=rec)
		except KeyboardInterrupt:
			raise
	elif options.filename:
		try:
			process(file=options.filename,mti=options.mti,ptype=plot)
		except KeyboardInterrupt:
			raise

if __name__ == '__main__':
	main()
