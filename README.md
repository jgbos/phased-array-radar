phased-array-radar
==================

Phased Array Radar

This repository was created to provide python enabled code to read data from MIT's [BUILD A SMALL PHASED ARRAY RADAR SENSOR.](http://web.mit.edu/professional/short-programs/courses/phased_array_radar_sensor.html)

Requires:

+ [SciPy](http://www.scipy.org/)
+ [NumPy](http://numpy.scipy.org/)
+ [pySerial](http://pyserial.sourceforge.net/)
+ [matplotlib](http://matplotlib.sourceforge.net/)

I recommend using the free downloadable python distribution by [Enthough](http://www.enthought.com/)

Usage
-----

In a Mac or Unix environment type the following in a terminal window:
	Usage: phased_array.py [options]

	Options:
  	-h, --help            show this help message and exit
  	-f FILE, --file=FILE  binary file to read from
  	-p PORT, --port=PORT  serial port to read from
  	--mti                 Turn on two pulse cancelor
  	--raw                 plot raw data
  	--raw-mti             plot raw data with two-pulse cancel
  	--rti-avg             plot RTI averaged across channels
  	--rti-max             plot RTI using max over angle bins in range-angle space
  	--range-angle         plot the range-angle data

