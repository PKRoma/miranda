import os
import sys
import string
import time
from vars import *

MODULES = ['Miranda-IM', 'SDK', 'Plugins', 'Protocols']
PROTOCOLS=[
	['IcqOscar8','icqoscar8 - Win32','icqoscar8.dsp'], 
	['AimTOC', 'aim - Win32','aim.dsp'], 
	['MSN', 'msn - Win32','msn.dsp'], 
	['Jabber','jabber - Win32','jabber.dsp']]

# functions

def cvs_get_module (module):		
	""" Sets up CVS_RSH to SSH and tells cvs to download the given module, SSH is set to a batch file which redirects to the real Putty Link """
	os.environ['CVS_RSH'] = 'util\\tp.bat' 		# setup CVS to use SSH, tp will redirect to tp.exe with password
	return os.system("Util\\cvs -z" + str(CVS_COMPRESSION_LEVEL) + " -d:ext:" + USER + "@cvs.sourceforge.net:/cvsroot/miranda-icq co " + module)
pass

def cvs_get_failed(module, status):
	""" Called to report error information during CVS stage of the script, does not return """
	print "Error: Failed to download " + module + "."
	sys.exit(status)
pass

def cvs_proto_compile(proto,mode):
	""" compiles a given protocol in a given compile mode, the path must be setup to the protocol build dir """
	return os.system("msdev " + proto[2] + ' /MAKE "' + proto[1] + ' ' + mode + '" /REBUILD')
pass

def cvs_module_compile(name,mode):
	""" compiles a given project name that must have its namesake in the same dir, i.e. import -> import.dsp """
	return os.system('msdev ' + name + '.dsp' + ' /MAKE "' + name + ' - Win32 ' + mode + '" /REBUILD');
pass

def cvs_get_changelog():
	os.environ['CVS_RSH'] = 'util\\tp.bat' 		# setup CVS to use SSH, tp will redirect to tp.exe with password
	rc=os.system('perl cvs2cl.pl -g -d:ext:' + USER + '@cvs.sourceforge.net:/cvsroot/miranda-icq')
	if rc != 0:
		print "Problem: Failed to create ChangeLog, you got perl?"
		sys.exit(rc)
pass

try:
	os.system('rmdir /S /Q Miranda-IM')
	os.system('rmdir /S /Q Protocols')
	os.system('rmdir /S /Q Plugins')
	os.system('rmdir /S /Q SDK')
	os.system('rmdir /S /Q Zip')
	os.system('rmdir /S /Q Bin')
	os.makedirs("Bin/Release/Plugins")
	os.makedirs("Bin/Debug/Plugins")
except:	
	pass	# ignore errors
	
# download every module

for m in MODULES:
	rc = cvs_get_module(m)		# fetch each module to disk
	if rc != 0:
		cvs_get_failed(m,rc)
pass

# create a zip of the source as-is
fp = open("package_src.py","r")
exec(fp.read())
fp.close()

# setup cvs and perl, to create ChangeLog, ugh..
cvs_get_changelog()

# compile Miranda [release]
os.chdir("Miranda-IM/")
rc = os.system("msdev miranda32.dsp /MAKE \"miranda32 - Win32 Release\" /REBUILD")
if rc != 0:
	print "Problem: Failed to compile Miranda [Release], status ::", rc
	sys.exit(rc)
	
# compile Miranda [debug]
rc = os.system("msdev miranda32.dsp /MAKE \"miranda32 - Win32 Debug\" /REBUILD")
if rc != 0:
	print "Problem: Failed to compile Miranda [Debug], status :: ", rc
	sys.exit(rc)
# check we got some binaries
try:
	os.stat("../Bin/Release/miranda32.exe")
	os.stat("../Bin/Debug/miranda32.exe")
except:
	print "Problem: Can not find miranda32.exe for release|debug mode"
	sys.exit(1)
pass

os.chdir('../Protocols/')  		# switch to protocol dir

for proto in PROTOCOLS:
	os.chdir(proto[0] + '/')					# go into proto dir
	rc=cvs_proto_compile(proto,"Release")	# compile protocol in release mode
	if rc != 0:
		print "Problem: Failed to compile",proto[0]," [Release] status ::", rc
		sys.exit(rc)			
	rc=cvs_proto_compile(proto,"Debug")		# compile protocol in debug mode
	if rc != 0:
		print "Problem: Failed to compile",proto[0],"[Debug] status ::", rc
		sys.exit(rc)
	os.chdir('../')				# leave proto dir
	
# setup mingw, assume its at C:\mingw\bin\, must be a better way to do this, ugh.
os.environ['PATH'] = os.environ['PATH'] + os.pathsep + "C:\\mingw\\bin";

os.chdir('Yahoo/')			# go into yahoo
try:
	os.mkdir("Build/")	# Make sure a build dir exists
except:
	pass
os.system("mingw32-make -f Makefile.win clean");	# run clean
rc = os.system("mingw32-make -f Makefile.win")			# run make (release)
rc=0
if rc != 0:
	print "Problem: Failed to compile Yahoo [Release], status ::", rc
	sys.exit(rc)
# compile debug version
rc = os.system("mingw32-make DEBUG=1 -f Makefile.win")	# run make (debug)
rc=0
if rc != 0:
	print "Problem: Failed to compile Yahoo [Debug], status ::", rc
	sys.exit(rc)

os.chdir('../../Plugins/import/')	# leave yahoo, leave protocol and goto plugin, import dir

# build import plugin (release)
rc = cvs_module_compile('import','Release')
if rc != 0:
	print "Problem: failed to compile import.dll [Release], status ::",rc
	sys.exit(rc)

# build import plugin (debug)
rc = cvs_module_compile('import', 'Debug')
if rc != 0:
	print "Problem: failed to compile import.dll [Debug], status ::",rc
	sys.exit(rc)
	
os.chdir('../changeinfo/')		# move out of import/ and into changeinfo/

# build changeinfo plugin (release)
rc = cvs_module_compile('changeinfo','Release')
if rc != 0:
	print "Problem: failed to compile changeinfo [Release], status ::",rc
	sys.exit(rc)

# build changeinfo plugin (debug)
rc = cvs_module_compile('changeinfo', 'Debug')
if rc != 0:
	print "Problem: failed to compile changeinfo [Debug], status ::",rc
	sys.exit(rc)
	
os.chdir('../srmm')				# move out of changeinfo, into srmm

# build srmm plugin (release)
rc = cvs_module_compile('srmm','Release')
if rc != 0:
	print "Problem: failed to compile srmm [Release], status ::",rc
	sys.exit(rc)

# build srmm plugin (debug)
rc = cvs_module_compile('srmm', 'Debug')
if rc != 0:
	print "Problem: failed to compile srmm [Debug], status ::",rc
	sys.exit(rc)

os.chdir('../../')				# move out of srmm back to root

# now build the packages

fp = open("package.py","r")
exec(fp.read())
fp.close()

# -------------------------------------------------------------------------------------
# The upload.py part of fetch.py
# -------------------------------------------------------------------------------------

if len(sys.argv) >= 3 and sys.argv[2] == "test":
	print "\n*** Build stage completed, skipping upload cos you passed 'test'"
	sys.exit(0)
	
import mrpage						# generate index

fp = open(sys.argv[1])				# read in the password file
p = string.strip(fp.read())
fp.close()

try:
	fp = open('upload.cmd', 'w')
except:
	print "Problem: Failed to create upload.cmd"
	sys.exit(1)
	
t = time.gmtime()
fp.write('cd /home/groups/m/mi/miranda-icq/htdocs/\n')
fp.write('mkdir alpha\n')
fp.write('chmod g+w alpha\n');
fp.write('cd alpha\n')
fp.write('put zip/' + MNAME + '.zip\n')
fp.write('put zip/' + MNAME + '_debug.zip\n')
fp.write('put zip/miranda_src.zip\n')
fp.write('put ChangeLog\n')
fp.write('put index.php\n')
fp.write('ls \n')
fp.close()

rc = os.system("util\\psftp -v -l " + USER + ' -pw ' + p  + ' -C -batch -b upload.cmd -be -bc shell.sourceforge.net')
if rc != 0:
	print "Problem! failed to create psftp connection..."
	sys.exit(rc)

print "\n*** Automatic build process complete."
sys.exit(0) # done