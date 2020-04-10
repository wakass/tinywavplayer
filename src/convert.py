#!/usr/local/opt/python/libexec/bin/python
import os
import sys

import wave
import numpy as np

def main(filename):
	base = os.path.basename(filename)
	dirname  = os.path.dirname(filename)
	name, ext = os.path.splitext(base)


	f = wave.open(filename)
	frames = f.getnframes()
	byteframes = f.readframes(frames)
	print("{}".format(f.getparams()))
	if f.getparams().sampwidth == 2:
	    dtype=np.int16
	else:
	    dtype = np.int8

	fr = np.frombuffer(byteframes,dtype=dtype)
	#If waveform is offset, center around 0
	fr = fr - np.mean(fr)
	fr = fr / np.max([np.max(fr),np.abs(np.min(fr))] )

	diffr = np.diff(fr)
	diffr[np.where(diffr > 0)] = 1.
	diffr[np.where(diffr <= 0)] = -1.

	ntotal = len(fr)
	x = fr
	y = np.zeros(ntotal)
	qe = 0 #running quantization error
	for n in range(1,len(fr)):
	    if x[n] >= qe:
	        y[n] = 1
	    else:
	        y[n] = -1
	    qe = y[n] - x[n] + qe
	    

	#pulsecode output
	y_packed = np.zeros(len(y))
	y_packed[np.where(y == 1)] = 1
	y_packed = y_packed.astype(np.int8)
	y_packed = np.packbits(y_packed)
	y_packed.tofile(name + '.pdm')


	reconstructed = np.zeros(len(y)+1)
	for n in range(1,len(y)):
	    reconstructed[n] = reconstructed[n-1] + y[n]*0.126 - 1/8*reconstructed[n-1]

	#wave output
	sig = reconstructed*2**16 #reconstitute for wav between 0 and maxint, 16bit
	f_out = wave.open(os.path.join(dirname,name + '_reconstructed' + ext) ,mode='wb')
	f_out.setparams(f.getparams())
	sig = sig.astype(np.int16)
	f_out.writeframesraw(memoryview(sig))

if __name__ == "__main__":

	filename = sys.argv[1] 
	main(filename)
