# mGBA 0.10.5 emulator core (MPL-2.0), vendored under 3rdparty/mgba.
#
# Only the GBA and GB cores are built, with every optional dependency
# (zlib, libpng, sqlite, lzma, ffmpeg, Lua, debuggers, GL renderers) off, so
# the app links nothing beyond glibc. The source list and defines mirror what
# mGBA's own CMake produces for a LIBMGBA_ONLY build with those options.
# 3rdparty/mgba-gen/version.c replaces the file CMake would generate.

MGBA = $$PWD/mgba

INCLUDEPATH += $$MGBA/include $$MGBA/src

QMAKE_CFLAGS += -std=gnu11 -Wno-unused-parameter -Wno-missing-field-initializers \
    -Wno-sign-compare -Wno-implicit-fallthrough -Wno-unused-function -Wno-unused-result
QMAKE_CFLAGS_RELEASE -= -O2
QMAKE_CFLAGS_RELEASE += -O3

DEFINES += M_CORE_GBA M_CORE_GB BUILD_STATIC USE_PTHREADS _GNU_SOURCE \
    HAVE_FREELOCALE HAVE_FUTIMENS HAVE_FUTIMES HAVE_LOCALE HAVE_LOCALTIME_R \
    HAVE_NEWLOCALE HAVE_PTHREAD_CREATE HAVE_PTHREAD_SETNAME_NP HAVE_SETLOCALE \
    HAVE_STRDUP HAVE_STRNDUP HAVE_USELOCALE HAVE_VASPRINTF

LIBS += -lpthread -lm

SOURCES += \
    $$MGBA/src/core/bitmap-cache.c \
    $$MGBA/src/core/cache-set.c \
    $$MGBA/src/core/cheats.c \
    $$MGBA/src/core/config.c \
    $$MGBA/src/core/core.c \
    $$MGBA/src/core/directories.c \
    $$MGBA/src/core/input.c \
    $$MGBA/src/core/interface.c \
    $$MGBA/src/core/library.c \
    $$MGBA/src/core/lockstep.c \
    $$MGBA/src/core/log.c \
    $$MGBA/src/core/map-cache.c \
    $$MGBA/src/core/mem-search.c \
    $$MGBA/src/core/rewind.c \
    $$MGBA/src/core/serialize.c \
    $$MGBA/src/core/sync.c \
    $$MGBA/src/core/thread.c \
    $$MGBA/src/core/tile-cache.c \
    $$MGBA/src/core/timing.c \
    $$MGBA/src/sm83/decoder.c \
    $$MGBA/src/sm83/isa-sm83.c \
    $$MGBA/src/sm83/sm83.c \
    $$MGBA/src/gb/audio.c \
    $$MGBA/src/gb/cheats.c \
    $$MGBA/src/gb/core.c \
    $$MGBA/src/gb/gb.c \
    $$MGBA/src/gb/input.c \
    $$MGBA/src/gb/io.c \
    $$MGBA/src/gb/mbc.c \
    $$MGBA/src/gb/memory.c \
    $$MGBA/src/gb/overrides.c \
    $$MGBA/src/gb/serialize.c \
    $$MGBA/src/gb/renderers/cache-set.c \
    $$MGBA/src/gb/renderers/software.c \
    $$MGBA/src/gb/sio.c \
    $$MGBA/src/gb/timer.c \
    $$MGBA/src/gb/video.c \
    $$MGBA/src/gb/sio/lockstep.c \
    $$MGBA/src/gb/sio/printer.c \
    $$MGBA/src/gb/extra/proxy.c \
    $$MGBA/src/arm/arm.c \
    $$MGBA/src/arm/decoder-arm.c \
    $$MGBA/src/arm/decoder.c \
    $$MGBA/src/arm/decoder-thumb.c \
    $$MGBA/src/arm/isa-arm.c \
    $$MGBA/src/arm/isa-thumb.c \
    $$MGBA/src/gba/audio.c \
    $$MGBA/src/gba/bios.c \
    $$MGBA/src/gba/cart/ereader.c \
    $$MGBA/src/gba/cart/gpio.c \
    $$MGBA/src/gba/cart/matrix.c \
    $$MGBA/src/gba/cart/vfame.c \
    $$MGBA/src/gba/cheats.c \
    $$MGBA/src/gba/cheats/codebreaker.c \
    $$MGBA/src/gba/cheats/gameshark.c \
    $$MGBA/src/gba/cheats/parv3.c \
    $$MGBA/src/gba/core.c \
    $$MGBA/src/gba/dma.c \
    $$MGBA/src/gba/gba.c \
    $$MGBA/src/gba/hle-bios.c \
    $$MGBA/src/gba/input.c \
    $$MGBA/src/gba/io.c \
    $$MGBA/src/gba/memory.c \
    $$MGBA/src/gba/overrides.c \
    $$MGBA/src/gba/renderers/cache-set.c \
    $$MGBA/src/gba/renderers/common.c \
    $$MGBA/src/gba/renderers/gl.c \
    $$MGBA/src/gba/renderers/software-bg.c \
    $$MGBA/src/gba/renderers/software-mode0.c \
    $$MGBA/src/gba/renderers/software-obj.c \
    $$MGBA/src/gba/renderers/video-software.c \
    $$MGBA/src/gba/savedata.c \
    $$MGBA/src/gba/serialize.c \
    $$MGBA/src/gba/sharkport.c \
    $$MGBA/src/gba/sio.c \
    $$MGBA/src/gba/sio/gbp.c \
    $$MGBA/src/gba/sio/joybus.c \
    $$MGBA/src/gba/sio/dolphin.c \
    $$MGBA/src/gba/sio/lockstep.c \
    $$MGBA/src/gba/timer.c \
    $$MGBA/src/gba/video.c \
    $$MGBA/src/gba/extra/audio-mixer.c \
    $$MGBA/src/gba/extra/battlechip.c \
    $$MGBA/src/gba/extra/proxy.c \
    $$MGBA/src/util/circle-buffer.c \
    $$MGBA/src/util/configuration.c \
    $$MGBA/src/util/crc32.c \
    $$MGBA/src/util/formatting.c \
    $$MGBA/src/util/gbk-table.c \
    $$MGBA/src/util/hash.c \
    $$MGBA/src/util/string.c \
    $$MGBA/src/util/table.c \
    $$MGBA/src/util/vfs.c \
    $$MGBA/src/util/convolve.c \
    $$MGBA/src/util/elf-read.c \
    $$MGBA/src/util/export.c \
    $$MGBA/src/util/patch.c \
    $$MGBA/src/util/patch-fast.c \
    $$MGBA/src/util/patch-ips.c \
    $$MGBA/src/util/patch-ups.c \
    $$MGBA/src/util/png-io.c \
    $$MGBA/src/util/ring-fifo.c \
    $$MGBA/src/util/text-codec.c \
    $$MGBA/src/util/vfs/vfs-mem.c \
    $$MGBA/src/util/vfs/vfs-fifo.c \
    $$MGBA/src/util/vfs/vfs-fd.c \
    $$MGBA/src/util/vfs/vfs-dirent.c \
    $$MGBA/src/platform/posix/memory.c \
    $$MGBA/src/third-party/inih/ini.c \
    $$MGBA/src/third-party/blip_buf/blip_buf.c \
    $$MGBA/src/feature/thread-proxy.c \
    $$MGBA/src/feature/video-logger.c \
    $$PWD/mgba-gen/version.c
