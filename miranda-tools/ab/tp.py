import os
import sys
import string

try:
	fp = open('PASSWORD', 'r')
except:
	print "Problem: PASSWORD file does not exist"
	sys.exit(1)
	pass
p = string.strip(fp.read())
fp.close()
args = string.join(sys.argv[3:(-2)])		# take everything off the command line after script, password file, then ignore cvs server
args = args + ' -pw ' + p + ' -batch -C ' + sys.argv[2] + ' cvs server'
os.system("util\\tp.exe " + args)