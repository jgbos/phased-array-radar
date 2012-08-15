

from scipy import hamming, fftpack
import numpy as np
import struct
import serial
import matplotlib.pyplot as plt
import radar

def get_nci(rci):
	"""
	Provides the average across the channels (like non-coherent integration)	
	"""
	shape = rci.shape
	rcs = np.sqrt( rci.real**2 + rci.imag**2)
 	return rcs.sum(0) / shape[0]
	
def get_angle(rci, nfft=128):
	"""
	Provides the Range-Angle image
	"""
	win = hamming(rci.shape[0])
	win.shape = (len(win),1)
	win = win.repeat(rci.shape[1],axis=1)
	return fftpack.fft(win*rci,nfft,axis=0)

def get_ci(rci, nfft=128):
	ati = get_angle(rci,nfft)
	shape = ati.shape
	s = ati.max(0)
	return np.sqrt( s.real**2 + s.imag**2)
	 
class DataDraw:
	def __init__(self, bufferSize=512, rangeGates=1024):
		self.fig = plt.figure()
		self.buffer = np.zeros((bufferSize,rangeGates))
		self.ny = bufferSize
		self.nx = rangeGates
		self.mode = 'rti'
		self.lastmode = None
		self.clim = [-30,10]	
		
		self.image = None	
		
	def set_mode(self,mode):
		self.lastmode = self.mode
		self.mode = mode

	def draw(self,rci):
		if self.mode == 'raw':
			self.set_data(rci)					
			ax = self.image.get_axes()
			ax.set_aspect(10)
			self.image.set_clim(self.clim)
			self.image.set_extent([0,rci.shape[1]-1,0,rci.shape[0]-1])
		elif self.mode == 'raw-mti':
			self.set_data(rci)					
			ax = self.image.get_axes()
			ax.set_aspect(10)
			self.image.set_clim(self.clim)
			self.image.set_extent([0,rci.shape[1]-1,0,rci.shape[0]-1])
		elif self.mode == 'rti':
			rti = get_nci(rci)
			self.update(rti)
			self.set_data(self.buffer)
			ax = self.image.get_axes()
			ax.set_aspect(2)
			self.image.set_clim(self.clim)
			self.image.set_extent([0,self.buffer.shape[1]-1,0,self.buffer.shape[0]-1])
		elif self.mode == 'ati':
			data = get_angle(rci)
			rcs = np.sqrt(data.real**2 + data.imag**2)
			val = 20*np.log10(rcs)
			self.set_data(val)
			ax = self.image.get_axes()
			ax.set_aspect(5)
			self.image.set_clim(self.clim)
			self.image.set_extent([0,val.shape[1]-1,0,val.shape[0]-1])
		elif self.mode == 'rti-ci':
			val = get_ci(rci)
			self.update(val)
			self.set_data(self.buffer)
			ax = self.image.get_axes()
			ax.set_aspect(2)
			self.image.set_clim([-20,10])
			self.image.set_extent([0,self.buffer.shape[1]-1,0,self.buffer.shape[0]-1])
		
		plt.draw()

	def update(self,rcs_m):
		# Update buffer for plotting RTI
		buffer = self.buffer
		n = buffer.shape[0]
		buffer[0:n-1,:] = buffer[1:n,:]
		buffer[n-1,:] = 20*np.log10(rcs_m)	
		
	def set_data(self,data):
		plt.figure(self.fig.number)
		if self.image is None:
			self.image = plt.imshow(data)
		else:
			self.image.set_data(data)

