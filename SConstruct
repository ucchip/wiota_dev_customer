import os
import sys
import rtconfig

if os.getenv('RTT_ROOT'):
    RTT_ROOT = os.getenv('RTT_ROOT')
else:
    RTT_ROOT = os.path.normpath(os.getcwd() + '/../../..')

# set RTT_ROOT
if not os.getenv("RTT_ROOT"): 
    #RTT_ROOT="rt-thread"
    RTT_ROOT="../../.."

# set RTT_ROOT
if not os.getenv("RTT_ROOT"): 
    RTT_ROOT="rt-thread"

sys.path = sys.path + [os.path.join(RTT_ROOT, 'tools')]
from building import *

TARGET = 'rtthread.' + rtconfig.TARGET_EXT

DefaultEnvironment(tools=[])
env = Environment(tools = ['mingw'],
    AS = rtconfig.AS, ASFLAGS = rtconfig.AFLAGS,
    CC = rtconfig.CC, CCFLAGS = rtconfig.CFLAGS,
    AR = rtconfig.AR, ARFLAGS = '-rc',
    LINK = rtconfig.LINK, LINKFLAGS = rtconfig.LFLAGS)
env.PrependENVPath('PATH', rtconfig.EXEC_PATH)
env['ASCOM'] = env['ASPPCOM']

Export('RTT_ROOT')
Export('rtconfig')
"""
# prepare building environment
objs = PrepareBuilding(env, RTT_ROOT)

# make a building
DoBuilding(TARGET, objs)
"""
SDK_ROOT = os.path.abspath('./')

if os.path.exists(SDK_ROOT + '/libraries'):
    libraries_path_prefix = SDK_ROOT + '/libraries'
else:
    libraries_path_prefix = os.path.dirname(SDK_ROOT) + '/libraries'

SDK_LIB = libraries_path_prefix
Export('SDK_LIB')

# prepare building environment
objs = PrepareBuilding(env, RTT_ROOT, has_libcpu=False)


# make a building
DoBuilding(TARGET, objs)
