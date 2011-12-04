#!/usr/bin/env python

import os

pack = open('BlackBeltSorviHero.dat', 'w')

length = 0
offset = 0
for i in [1, 2]:
	length = 0
	for fname in os.listdir('data'):
		f = open('data/' + fname)
		f.seek(0, 2)
		size = f.tell()
		f.close()
		length += len("%s %d %d\n" % (fname, offset, size))
		offset += size
	offset = length + 4

for fname in os.listdir('data'):
	f = open('data/' + fname)
	f.seek(0, 2)
	size = f.tell()
	f.close()
	pack.write("%s %d %d\n" % (fname, offset, size))
	offset += size
pack.write("END\n")

for fname in os.listdir('data'):
	f = open('data/' + fname)
	while True:
		s = f.read(4096)
		if not s:
			break
		pack.write(s)
	size = f.tell()
	f.close()

pack.close()
