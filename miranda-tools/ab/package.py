import sys
import os
import string
import zipfile
import time
import os.path
from vars import *

try:
	z = zipfile.ZipFile('zip/' + MNAME + '.zip','w',zipfile.ZIP_DEFLATED)	
except:
	print "Problem: Failed to create nightly zip (release)"
	sys.exit(1)
	
print "creating", MNAME + '_zip', "(release) mode"
z.write('Bin/Release/miranda32.exe', 'miranda32.exe')
z.write('Bin/Release/Plugins/ICQ.dll', 'Plugins/ICQ.dll')
z.write('Bin/Release/Plugins/AIM.dll','Plugins/AIM.dll')
z.write('Bin/Release/Plugins/msn.dll','Plugins/msn.dll')
z.write('Bin/Release/Plugins/jabber.dll','Plugins/jabber.dll')
z.write('Bin/Release/Plugins/Yahoo.dll','Plugins/Yahoo.dll')
z.write('Bin/Release/Plugins/import.dll','Plugins/import.dll')
z.write('Bin/Release/Plugins/changeinfo.dll','Plugins/changeinfo.dll')
z.write('Bin/Release/Plugins/srmm.dll','Plugins/srmm.dll')
z.close()
z=0

try:
	z = zipfile.ZipFile('zip/' + MNAME + '_debug.zip','w',zipfile.ZIP_DEFLATED)	
except:
	print "Problem: Failed to create nightly zip (debug)"
	sys.exit(1)
	
print "creating", MNAME + '_debug.zip', "(debug) mode"
z.write('Bin/Debug/miranda32.exe', 'miranda32.exe')
z.write('Bin/Debug/Plugins/ICQ.dll', 'Plugins/ICQ.dll')
z.write('Bin/Debug/Plugins/AIM.dll','Plugins/AIM.dll')
z.write('Bin/Debug/Plugins/msn.dll','Plugins/msn.dll')
z.write('Bin/Debug/Plugins/jabber.dll','Plugins/jabber.dll')
z.write('Bin/Debug/Plugins/Yahoo.dll','Plugins/Yahoo.dll')
z.write('Bin/Debug/Plugins/import.dll','Plugins/import.dll')
z.write('Bin/Debug/Plugins/changeinfo.dll','Plugins/changeinfo.dll')
z.write('Bin/Debug/Plugins/srmm.dll','Plugins/srmm.dll')
z.close()
z=0

