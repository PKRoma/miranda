import sys
import os
import string
import zipfile
import os.path

def copytree(z,src):
	""" Stolen from shutils, copytree example, adds all files found to the given ZipFile object """
	names = os.listdir(src)
	for name in names:
		srcname = os.path.join(src, name)
		try:
			if os.path.isdir(srcname):
				copytree(z,srcname)
			else:
				if string.find(srcname,"CVS") == (-1):	# can't be bothered dealing with all the compiler output crap
					z.write(srcname)
		except:
			pass
			
try:
	os.mkdir("zip")	
except:
	pass
	
try:
	z = zipfile.ZipFile('zip/miranda_src.zip','w',zipfile.ZIP_DEFLATED)
except:
	print "Problem: Failed to create nightly zip (source)"
	sys.exit(1)

print "creating miranda_src.zip"

copytree(z,'Miranda-IM')
copytree(z,'Protocols')
copytree(z,'Plugins')
copytree(z,'SDK')

z.close()
z=0
