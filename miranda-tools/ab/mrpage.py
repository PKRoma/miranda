import os
import sys
import string
import time
from vars import *

try:
	f = open('template_index.tpl','r')
except:
	print "Problem, failed to open template_index.tpl"
	sys.exit(1)
	
dlstr = """
	<a href="%s">%s</a>
	<a href="%s">%s</a>
	<a href="%s">%s</a>
	""" % (MNAME + '.zip', MNAME + '.zip', MNAME + '_debug.zip', MNAME + '_debug.zip', 'miranda_src.zip', 'miranda_src.zip')
statstr = 'Generated on ' + time.strftime('%Y/%m/%d at %H:%M:%S') + ' by ' + USER
notestr = 'No extra information included'

try:
	n = open('NOTES','r')
	notestr = n.read()
	n.close()
except:
	pass
	
page = f.read()										 # read in the page
page = string.replace(page,'%%DOWNLOAD%%',dlstr)	 # replace %%DOWNLOAD%% with index
page = string.replace(page,'%%DEVELNOTES%%',notestr) # replace %%DEVELNOTES%% with str
page = string.replace(page,'%%STATS%%',statstr)		 # replace %%STATS%% with some info
f.close() 	# close template page

try:
	fp = open('index.php','w+')
except:
	print "Problem, unable to create index.php!"
	sys.exit(1)
	
fp.write(page)		# write out the generated page to index.php
fp.close()
