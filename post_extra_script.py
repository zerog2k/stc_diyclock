''' custom script for platformio '''

from os.path import join
from SCons.Script import DefaultEnvironment

env = DefaultEnvironment()

#print "post_extra_script running..."
#print env.Dump()

# compiler and linker flags dont work very well in build_flags of platformio.ini - need to set them here
env.Append(
    CFLAGS = [
    "--disable-warning", 126,
    "--disable-warning", 59
    ]
)

