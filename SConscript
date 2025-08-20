from building import *

pwd     = GetCurrentDir()
src     = []
CPPPATH = []

src = Glob('src/*.c')
src += Glob('osal/rtthread.c')
CPPPATH = [pwd + '/inc']

if GetDepend(['PKG_USING_MCOREDUMP_EXAMPLE']):
    src += Glob('example/*.c')

if GetDepend(['PKG_USING_MCOREDUMP_ARCH_ARMV7M']):
    src += Glob('arch/armv7m/*.c')
    CPPPATH.append(pwd + '/arch/armv7m')

    src += Glob('src/arm/*.c')
    CPPPATH.append(pwd + '/src/arm')

if GetDepend(['PKG_USING_MCOREDUMP_ARCH_ARMV8M']):
    src += Glob('arch/armv8m/*.c')
    CPPPATH.append(pwd + '/arch/armv8m')

    src += Glob('src/arm/*.c')
    CPPPATH.append(pwd + '/src/arm')

CPPPATH.append(pwd + '/arch')

group = DefineGroup('MCoreDump', src, depend = ['PKG_USING_MCOREDUMP'], CPPPATH = CPPPATH)

Return('group')