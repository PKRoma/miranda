import os
import sys
import string
from vars import *

fp = open(sys.argv[1])				# read in the password file
p = string.strip(fp.read())
fp.close()

try:
	fp = open('rich.cmd', 'w')
except:
	print "Problem: Failed to create rich.cmd"
	sys.exit(1)
	
t = time.gmtime()
fp.write('cd /home/groups/m/mi/miranda-icq/htdocs/\n')
fp.write('mkdir alpha\n')
fp.write('chmod g+w alpha\n');
fp.write('cd alpha\n')
fp.write('ln -sf ' + MNAME + '.zip' + ' miranda_nightly.zip\n')
fp.write('ln -sf ' + MNAME + '_debug.zip' + ' miranda_nightly-debug.zip\n')
fp.write('ln -sf ChangeLog changelog.txt\n')
fp.write('ln -sf ChangeLog changelogproto.txt\n')
fp.write('ln -sf ChangeLog changelogplugins.txt\n')
fp.write('md5sum ' + MNAME + '* > MD5SUMS \n');
fp.close()

rc=os.system("util\putty -ssh -m rich.cmd -C -v -pw " + p + ' ' + USER + '@shell.sourceforge.net')
if rc != 0:
	print "Problem: Unable to start PuTTy, status ::", rc
	sys.exit(rc)
	