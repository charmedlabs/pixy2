# This program uses pypng, https://github.com/drj11/pypng
#
# These barcodes are 4-bit manchester-encoded codes.  All codes 
# start with a black bar to establish the scale.  Some codes include
# a black bar to end the code if necessary. 

import png

barWidth = 50 # pixels
barHeight = 7 # width multiplier
bitWidth = 4 # number of bits in barcode

barHeight *= barWidth

def bar(color, width, height):
	row = [color&1] * width
	bar = [row] * height
	return bar

def concat(a, b):
	c = []
	for r in range(len(a)):
		c.append(a[r] + b[r])
	return c

def buildCode(val, bits, width, height):
	# Add black bar
	blackBars = 1
	color = 0
	code =  bar(color, width, height)
	color += 1
	prevBit = 1

	for i in range(bits): 
		bit = (val & (1<<(bits-1)))>>(bits-1)
		if prevBit^bit:
			code = concat(code, bar(color, width*2, height))
			if (color&1)==0:
				blackBars += 1
		else:
			code = concat(code, bar(color, width, height))
			color += 1
			code = concat(code, bar(color, width, height))
			blackBars += 1

			
		color += 1
		prevBit = bit
		val = val << 1
	
	# add guard bar if last bar is white
	if (color&1)==0:
		code = concat(code, bar(color, width, height))
		blackBars += 1
		
	# barcode should have at least as many black bars as bits for detection purposes
	if blackBars<bits:
		code = concat(code, bar(color, width, height))
		color += 1
		code = concat(code, bar(color, width, height))
	
	return code
	
def writeCode(filename, val, bits, width, height):
	code = buildCode(val, bits, width, height)
	f = open(filename + str(val) + '.png', 'wb')
	w = png.Writer(len(code[0]), len(code), greyscale=True, bitdepth=1)
	w.write(f, code)
	f.close()	


for i in range(1<<bitWidth):
	writeCode('barcode', i, bitWidth, barWidth, barHeight)
