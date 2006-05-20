#! /usr/bin/env python

import os
import string
import sys
import socket

#
# these are all the modules we want
#
CVS_MODULES = ['miranda']

#
# make sure the script is running in /home/egodust/ab/
#

os.chdir('/home/egodust/ab')


#
# make sure cvs uses SSH
# 
os.environ['CVS_RSH'] = 'ssh'

#
# fetch all the modules to disk, Q = quiet, r = store all files as read only, group/others = no access
#
for m in CVS_MODULES:
	rc = os.system('/usr/bin/cvs -z5 -d:ext:miranda_ab@cvs.sourceforge.net:/cvsroot/miranda checkout ' + m)
	if rc != 0:
		print 'Failed to CVS checkout ' + m
		sys.exit(rc)
		
#
# store the date (might end up approx) when checkout happened
# 
rc = os.system('date --iso-8601 +%F%t%H:%M:%S%t%Z | cat > checkout.when')
if rc != 0:
	print 'Failed to store checkout.when'
	sys.exit(rc)
	
#
# find all *.dsp files and convert all LF to CRLF since MSVC++ cant handle it
#
rc = os.system('find -type f -iname *.dsp -exec unix2dos --quiet {} \;')
if rc != 0:
	print 'Failed to find/convert *.dsp files'
	sys.exit(rc)

rc = os.system('find -type f -iname *.txt -exec unix2dos --quiet {} \;')
if rc != 0:
	print 'Failed to find/convert *.txt'
	sys.exit(rc)

#rc = os.system('tar --create --bzip --verbose --file=miranda_cvs_tmp.bz2 --exclude CVS --exclude CVSROOT --exclude Tools/ab/Util ' + string.join(CVS_MODULES) )
rc = os.system('zip -X -q -9 -r miranda_cvs_tmp.zip ' + string.join(CVS_MODULES) + ' checkout.when -x CVS CVSROOT Tools/ab/Util/*');
if rc != 0:
	print 'Failed to generate the archives.'
	sys.exit(rc)
	
rc = os.system('mv miranda_cvs_tmp.zip miranda_cvs.zip --reply=yes')
if rc != 0:
	print 'Failed to move miranda_cvs_tmp.zip to miranda_cvs.zip'
	sys.exit(rc)

rc = os.system('chmod 0644 miranda_cvs.zip')
if rc != 0:
	print 'Failed to switch archive to safer permissions.'
	sys.exit(rc)
