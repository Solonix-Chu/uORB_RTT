from building import *
import os

cwd = GetCurrentDir()

# uORB Library configuration
src = []
CPPPATH = []
CPPDEFINES = ['UORB_ENABLED']

# Include paths
CPPPATH += [cwd + '/inc']
CPPPATH += [cwd + '/src/tools']

# Core source files
core_src = [
    'src/uorb_device_node.c',
    'src/uorb_core.c',
    'src/uorb_utils.c',
    'src/uorb_cli.c',
    'src/uorb_device_if.c',
    'src/uorb_print.c',
]

# Optional: .msg generation
if GetDepend(['UORB_USING_MSG_GEN']):
    msg_dir = os.path.join(cwd, 'msg')
    inc_topics = os.path.join(cwd, 'inc', 'topics')
    meta_dir = os.path.join(cwd, 'src', 'metadata')
    try:
        os.makedirs(inc_topics, exist_ok=True)
        os.makedirs(meta_dir, exist_ok=True)
    except Exception:
        pass
    # Run generator
    gen_script = os.path.join(cwd, 'tools', 'msggen.py')
    ret = os.system('python3 "{}"'.format(gen_script))
    if ret == 0:
        print('[uorb] msggen ok -> collecting generated sources')
        # Collect generated sources
        for f in os.listdir(meta_dir):
            if f.endswith('.c'):
                core_src.append('src/metadata/' + f)
        # Add include path for generated headers
        CPPPATH += [inc_topics]
        # Mark that topics were generated and generator is enabled
        CPPDEFINES.append('UORB_TOPICS_GENERATED')
    else:
        print('[uorb] msggen failed (ret={}), fallback to demo topics'.format(ret))
        # Fallback to hand-coded demo metadata
        core_src.append('src/uorb_demo_topics.c')
else:
    # Fallback to hand-coded demo metadata
    core_src.append('src/uorb_demo_topics.c')

# Add core sources
src += core_src

# Test source files (only if testing is enabled)
if GetDepend(['PKG_USING_UTEST']) or GetDepend(['RT_USING_UTEST']):
    try:
        utest_group = SConscript(os.path.join('test', 'unit', 'utest', 'SConscript'))
    except Exception:
        # Fallback: include all test sources under utest
        test_dir = os.path.join('test', 'unit', 'utest')
        src += Glob(os.path.join(test_dir, '*.c'))
        CPPPATH += [cwd + '/test/unit/utest']

# Define the group
# Enable C++ wrapper include path when configured
if GetDepend(['UORB_USING_CXX']):
    CPPPATH += [cwd + '/inc/cxx']
    LOCAL_CXXFLAGS = ' -fno-exceptions -fno-rtti -fno-unwind-tables -fno-asynchronous-unwind-tables'
else:
    LOCAL_CXXFLAGS = ''

group = DefineGroup('uORB', src, depend = [''], CPPPATH = CPPPATH, CPPDEFINES = CPPDEFINES, LOCAL_CXXFLAGS = LOCAL_CXXFLAGS)

# attach utest group if present
try:
    group = group + utest_group
except Exception:
    pass

# attach examples under uORB
try:
    ex_group = SConscript(os.path.join('examples', 'SConscript'))
    group = group + ex_group
except Exception:
    pass

Return('group') 