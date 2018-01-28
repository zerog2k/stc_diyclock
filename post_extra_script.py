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
    ],
    LINKFLAGS = [
        "--data-loc", 0x30
    ]
)

# best way to set (local) stcgal as uploader with correct options is here
if "BOARD" in env and env.BoardConfig().get("vendor") == "STC":
    f_cpu_khz = int(env.BoardConfig().get("build.f_cpu")) / 1000
    if env.BoardConfig().get("build.variant") == "stc15f204ea":
        stc_protocol = "stc15a"
    else:
        stc_protocol = "auto"
    
    env.Replace(
            UPLOAD_OPTIONS = [
                "-p", "$UPLOAD_PORT",
                "-P", stc_protocol,
                "-t", int(f_cpu_khz),
            ],
            UPLOADHEXCMD = "stcgal/stcgal.py $UPLOAD_OPTIONS $UPLOAD_FLAGS $SOURCE"
    )
