import time

USER='egodust'
t = time.gmtime()
MNAME='miranda_' + str(t[0]).zfill(4) + str(t[1]).zfill(2) + str(t[2]).zfill(2) # build the name using yyyymmdd format
CVS_COMPRESSION_LEVEL=7
