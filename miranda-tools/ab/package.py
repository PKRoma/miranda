import sys
import os
import string
import zipfile
import time
import os.path

try:
	t = time.gmtime()
	name = 'miranda_' + str(t[0]).zfill(4) + str(t[1]).zfill(2) + str(t[2]).zfill(2) + '.zip' # build the name using yyyymmdd format
	z = zipfile.ZipFile('zip/' + name,'w',zipfile.ZIP_DEFLATED)	
except:
	print "Problem: Failed to create nightly zip (release)"
	sys.exit(1)
	
print "creating", name, "(release) mode"
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
	t = time.gmtime()
	name = 'miranda_' + str(t[0]).zfill(4) + str(t[1]).zfill(2) + str(t[2]).zfill(2) + '_debug.zip' # build the name using yyyymmdd format
	z = zipfile.ZipFile('zip/' + name,'w',zipfile.ZIP_DEFLATED)	
except:
	print "Problem: Failed to create nightly zip (debug)"
	sys.exit(1)
	
print "creating", name, "(debug) mode"
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

