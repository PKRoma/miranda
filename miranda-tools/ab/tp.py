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

# reformat args to be: username, pass, options <server> <commands>
args = '-l ' + sys.argv[3] + ' -pw ' + p + ' -batch -C ' + sys.argv[1] + ' ' + string.join( sys.argv[4:] )
os.system("util\\tp.exe " + args)