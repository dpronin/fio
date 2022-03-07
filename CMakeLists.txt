cmake_minimum_required(VERSION 3.25 FATAL_ERROR)
project(fio
        VERSION 3.35.0
        HOMEPAGE_URL "https://github.com/axboe/fio"
        LANGUAGES C CXX
        DESCRIPTION "Flexible I/O Tester")

option(ENABLE_PDB "Enable Windows PDB symbols generation (needs clang/lld)")
option(BUILD_STATIC "Build a static fio")
option(ESX "Configure build options for esx")
option(ENABLE_GFIO "Enable building of gtk gfio")
option(DISABLE_NUMA "Disable libnuma even if found")
option(DISABLE_RDMA "Disable RDMA support even if found")
option(DISABLE_RADOS "Disable Rados support even if found")
option(DISABLE_RBD "Disable Rados Block Device even if found")
option(DISABLE_HTTP "Disable HTTP support even if found")
option(DISABLE_GFAPI "Disable gfapi")
option(ENABLE_LIBHDFS "Enable hdfs support")
option(ENABLE_LIBNFS "Enable nfs support")
option(DISABLE_LIBNFS "Disable nfs support")
option(DISABLE_LEX "Disable use of lex/yacc for math")
option(DISABLE_PMEM "Disable pmem based engines even if found")
option(ENABLE_LEX "Enable use of lex/yacc for math")
option(DISABLE_SHM "Disable SHM support")
option(DISABLE_OPTIMIZATIONS "Don't enable compiler optimizations")
option(ENABLE_CUDA "Enable GPUDirect RDMA support")
option(ENABLE_LIBCUFILE "Enable GPUDirect Storage cuFile support")
option(DISABLE_NATIVE "Don't build for native host")
option(ENABLE_LIBISCSI "Enable iscsi support")
option(ENABLE_LIBNBD "Enable libnbd (NBD engine) support")
option(DISABLE_XNVME "Disable xnvme support even if found")
option(DISABLE_LIBBLKIO "Disable libblkio support even if found")
option(DISABLE_LIBZBC "Disable libzbc even if found")
option(DISABLE_TCMALLOC "Disable tcmalloc support")
option(DYNAMIC_LIBENGINES "Lib-based ioengines as dynamic libraries")
option(DISABLE_DFS "Disable DAOS File System support even if found")
option(ENABLE_ASAN "Enable address sanitizer")
if (NOT SEED_BUCKETS)
    set(SEED_BUCKETS 4)
endif ()
if (NOT "${SEED_BUCKETS}" MATCHES "[1-9][0-9]*")
    set(SEED_BUCKETS 4)
elseif (${SEED_BUCKETS} LESS 2)
    set(SEED_BUCKETS 2)
elseif (${SEED_BUCKETS} GREATER 16)
    set(SEED_BUCKETS 16)
endif ()
set(SEED_BUCKETS ${SEED_BUCKETS} CACHE STRING "Number of seed buckets for the refill-buffer" FORCE)
option(DISABLE_TLS "Disable __thread local storage")

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

set(CONFIG_NO_SHM ${DISABLE_SHM})
set(CONFIG_FIO_NO_OPT ${DISABLE_OPTIMIZATIONS})
set(CONFIG_DISABLE_OPTIMIZATIONS ${DISABLE_OPTIMIZATIONS})
set(PTHREAD_CONDATTR_SETCLOCK YES)

find_package(Threads REQUIRED)

set(FIO_LIBRARIES
    m
    Threads::Threads
    ${CMAKE_DL_LIBS}
    $<$<BOOL:$<PLATFORM_ID:Linux>>:rt>
    ${EXTLIBS}
)

# include(GNUInstallDirs)
include(TestBigEndian)
include(CheckCSourceCompiles)
include(CheckCSourceRuns)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Android")
    set(CONFIG_TARGET_OS "Android")
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(CONFIG_TARGET_OS "Linux")
elseif (${BSD} MATCHES "OpenBSD")
    set(CONFIG_TARGET_OS "OpenBSD")
elseif (${BSD} MATCHES "NetBSD")
    set(CONFIG_TARGET_OS "NetBSD")
elseif (CMAKE_HOST_SOLARIS)
    set(CONFIG_TARGET_OS "SunOS")
    add_compile_definitions(_REENTRANT)
elseif (CYGWIN)
    set(CONFIG_TARGET_OS "CYGWIN")
else ()
    execute_process(COMMAND bash -c "printf $(uname -s)" OUTPUT_VARIABLE CONFIG_TARGET_OS)
endif ()

# Some host OSes need non-standard checks for which CPU to use.
# Note that these checks are broken for cross-compilation: if you're
# cross-compiling to one of these OSes then you'll need to specify
# the correct CPU with the --cpu option.
if (${CONFIG_TARGET_OS} MATCHES "AIX|OpenBSD|NetBSD")
    # Unless explicitly enabled, turn off lex.
    # OpenBSD will hit syntax error when enabled.
    if (DISABLE_LEX)
        set(FORCE_NO_LEX_O YES)
    endif ()
elseif (${CONFIG_TARGET_OS} MATCHES "FreeBSD")
    include_directories(/usr/local/include)
    add_link_options(-L/usr/local/lib)
elseif (${CONFIG_TARGET_OS} MATCHES "Darwin")
    # # on Leopard most of the system is 32-bit, so we have to ask the kernel if
    # # we can run 64-bit userspace code.
    # # If the user didn't specify a CPU explicitly and the kernel says this is
    # # 64 bit hw, then assume x86_64. Otherwise fall through to the usual
    # # detection code.
    # if test -z "$cpu" && test "$(sysctl -n hw.optional.x86_64)" = "1"; then
    #   cpu="x86_64"
    # fi
    # # Avoid configure feature detection of features provided by weak symbols
elseif (${CONFIG_TARGET_OS} MATCHES "SunOS")
    # # `uname -m` returns i86pc even on an x86_64 box, so default based on isainfo
    # if test -z "$cpu" && test "$(isainfo -k)" = "amd64"; then
    #   cpu="x86_64"
    # fi
    list(APPEND FIO_LIBRARIES nsl socket)
elseif (${CONFIG_TARGET_OS} MATCHES "CYGWIN")
    # We need this to be output_sym'd here because this is Windows specific.
    # The regular configure path never sets this config.
    set(CONFIG_WINDOWSAIO True)
    # We now take the regular configuration path without having exit 0 here.
    # Flags below are still necessary mostly for MinGW.
    set(BUILD_STATIC YES)
    set(RUSAGE_THREAD YES)
    set(FDATASYNC YES)
    set(CLOCK_GETTIME YES) # clock_monotonic probe has dependency on this
    set(CLOCK_MONOTONIC YES)
    set(SCHED_IDLE YES)
    set(PTHREAD_CONDATTR_SETCLOCK NO)
    set(PTHREAD_AFFINITY NO)
endif ()

set(CPU ${CMAKE_HOST_SYSTEM_PROCESSOR})

set(CMAKE_C_STANDARD 99)
set(CMAKE_EXPORT_COMPILE_COMMANDS true)
set(CMAKE_C_STANDARD_REQUIRED on)

add_compile_definitions(
    _GNU_SOURCE
    _LARGEFILE_SOURCE
    _FILE_OFFSET_BITS=64
    FIO_INTERNAL
    FIO_VERSION="${PROJECT_VERSION}"
    FIO_INC_DEBUG
)

add_compile_options(
    -Wimplicit-fallthrough
    -fno-builtin-stpcpy
    -g
    -ffast-math
    -Wwrite-strings
    -Wall
    -Wdeclaration-after-statement
    $<$<BOOL:${BUILD_STATIC}>:-ffunction-sections>
    $<$<BOOL:${BUILD_STATIC}>:-fdata-sections>
)

add_link_options(
    $<$<BOOL:${BUILD_STATIC}>:-static>
    $<$<BOOL:${BUILD_STATIC}>:-Wl,--gc-sections>
)

if (${CMAKE_C_BYTE_ORDER} STREQUAL "BIG_ENDIAN")
    set(CONFIG_BIG_ENDIAN YES)
    set(CONFIG_LITTLE_ENDIAN NO)
else ()
    set(CONFIG_BIG_ENDIAN NO)
    set(CONFIG_LITTLE_ENDIAN YES)
endif ()

message(STATUS "Operating system " ${CONFIG_TARGET_OS})
message(STATUS "CPU " ${CPU})
message(STATUS "Big endian " ${CONFIG_BIG_ENDIAN})
message(STATUS "Compiler " ${CMAKE_C_COMPILER})
# message(STATUS "Cross compile" "$cross_compile")
message(STATUS "Static build " ${BUILD_STATIC})

include_directories(${CMAKE_CURRENT_LIST_DIR})

set(FIO_SOURCES
    backend.cpp
    blktrace.cpp
    cconv.cpp
    cgroup.cpp
    client.cpp
    crc/crc16.cpp
    crc/crc32.cpp
    crc/crc32c-arm64.cpp
    crc/crc32c-intel.cpp
    crc/crc32c.cpp
    crc/crc64.cpp
    crc/crc7.cpp
    crc/fnv.cpp
    crc/md5.cpp
    crc/murmur3.cpp
    crc/sha1.cpp
    crc/sha256.cpp
    crc/sha3.cpp
    crc/sha512.cpp
    crc/test.cpp
    crc/xxhash.cpp
    debug.cpp
    dedupe.cpp
    diskutil.cpp
    engines/cpu.c
    engines/exec.c
    engines/fileoperations.c
    engines/ftruncate.c
    engines/mmap.c
    engines/net.c
    engines/null.c
    engines/sync.c
    eta.cpp
    fdp.cpp
    filehash.cpp
    filelock.cpp
    filesetup.cpp
    fio_sem.cpp
    flow.cpp
    gettime-thread.cpp
    gettime.cpp
    helper_thread.cpp
    helpers.cpp
    idletime.cpp
    init.cpp
    io_u.cpp
    io_u_queue.cpp
    ioengines.cpp
    iolog.cpp
    json.cpp
    lib/axmap.cpp
    lib/bloom.cpp
    lib/flist_sort.cpp
    lib/gauss.cpp
    lib/getrusage.cpp
    lib/hweight.cpp
    lib/ieee754.cpp
    lib/lfsr.cpp
    lib/memalign.cpp
    lib/memcpy.cpp
    lib/mountcheck.cpp
    lib/num2str.cpp
    lib/output_buffer.cpp
    lib/pattern.cpp
    lib/prio_tree.cpp
    lib/rand.cpp
    lib/rbtree.cpp
    lib/strntol.cpp
    lib/zipf.cpp
    libfio.cpp
    log.cpp
    memory.cpp
    optgroup.cpp
    options.cpp
    oslib/linux-dev-lookup.cpp
    parse.cpp
    profile.cpp
    profiles/act.cpp
    profiles/tiobench.cpp
    pshared.cpp
    rate-submit.cpp
    rwlock.cpp
    server.cpp
    smalloc.cpp
    stat.cpp
    steadystate.cpp
    td_error.cpp
    time.cpp
    trim.cpp
    verify.cpp
    workqueue.cpp
    zbd.cpp
    zone-dist.cpp
)

set(CMAKE_REQUIRED_INCLUDES_DEFAULT)
set(CMAKE_REQUIRED_FLAGS_DEFAULT -Werror-implicit-function-declaration)
set(CMAKE_REQUIRED_LIBRARIES_DEFAULT)
set(CMAKE_REQUIRED_DEFINITIONS_DEFAULT -D_GNU_SOURCE)

macro(compile_c_program_and_check NAME SOURCE INCLUDES FLAGS DEFINITIONS LIBRARIES CHECK_VAR)
    set(CMAKE_REQUIRED_QUIET YES)
    set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES_DEFAULT} ${INCLUDES})
    set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS_DEFAULT} ${FLAGS}")
    set(CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS_DEFAULT} ${DEFINITIONS})
    set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES_DEFAULT} ${LIBRARIES})
    unset(${CHECK_VAR})
    check_c_source_compiles("${${SOURCE}}" ${CHECK_VAR})
    unset(CMAKE_REQUIRED_DEFINITIONS)
    unset(CMAKE_REQUIRED_LIBRARIES)
    unset(CMAKE_REQUIRED_FLAGS)
    unset(CMAKE_REQUIRED_INCLUDES)
    unset(CMAKE_REQUIRED_QUIET)
    if (${CHECK_VAR})
        set(${CHECK_VAR} YES)
    else ()
        set(${CHECK_VAR} NO)
    endif()
    message(NOTICE "${NAME} ${${CHECK_VAR}}")
endmacro()

function(EXEC prog RESULT)
    execute_process(COMMAND "${prog}" RESULT_VARIABLE ${RESULT} OUTPUT_QUIET)
    if (${${RESULT}} EQUAL 0)
        set(${RESULT} YES)
    else ()
        set(${RESULT} NO)
    endif ()
    return(PROPAGATE ${RESULT})
endfunction()

function(HAS prog RESULT)
    EXEC("bash -c \"type ${prog}\"" ${RESULT})
    return(PROPAGATE ${RESULT})
endfunction()

##########################################
# check for C11 atomics support

set(C11_ATOMICS_CHECK_CODE
    "
    #include <stdatomic.h>
    int main(void)
    {
      _Atomic unsigned v;
      atomic_load(&v);
      return 0;
    }
    "
)
compile_c_program_and_check("c11_atomics" C11_ATOMICS_CHECK_CODE "" "" "" "" HAS_C11_ATOMICS)
if (NOT HAS_C11_ATOMICS)
    message(FATAL_ERROR
            "Your compiler doesn't support C11 atomics. gcc 4.9/clang 3.6 are the "
            "minimum versions with it - perhaps your compiler is too old?\n"
            "C11 atomics support not found")
endif ()

##########################################
# check for wordsize

set(WORDSIZE_CHECK_CODE
    "
    #include <limits.h>
    #define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))
    int main(void)
    {
      BUILD_BUG_ON(sizeof(long)*CHAR_BIT != WORDSIZE);
      return 0;
    }
    "
)
compile_c_program_and_check("Wordsize-32" WORDSIZE_CHECK_CODE "" "" "-DWORDSIZE=32" "" HAS_WORDSIZE_32)
if (HAS_WORDSIZE_32)
    set(WORDSIZE 32)
    set(CONFIG_32BIT YES)
else ()
    compile_c_program_and_check("Wordsize-64" WORDSIZE_CHECK_CODE "" "" "-DWORDSIZE=64" "" HAS_WORDSIZE_64)
    if (HAS_WORDSIZE_64)
        set(WORDSIZE 64)
        set(CONFIG_64BIT YES)
    else()
        message(FATAL_ERROR "Unknown wordsize!")
    endif ()
endif ()

add_compile_definitions(BITS_PER_LONG=${WORDSIZE})

##########################################
# zlib probe

set(ZLIB_CHECK_CODE
    "
    #include <zlib.h>
    int main(void)
    {
      z_stream stream;
      if (inflateInit(&stream) != Z_OK)
        return 1;
      return 0;
    }
    "
)
compile_c_program_and_check("zlib" ZLIB_CHECK_CODE "" "" "-DWORDSIZE=64" "-lz" CONFIG_ZLIB)
if (CONFIG_ZLIB)
    list(APPEND FIO_LIBRARIES z)
endif ()

##########################################
# fcntl(F_FULLFSYNC) support

set(FCNTL_SYNC_CODE
    "
    #include <unistd.h>
    #include <fcntl.h>

    int main(int argc, char **argv)
    {
      return fcntl(0, F_FULLFSYNC);
    }
    "
)
compile_c_program_and_check("fcntl(F_FULLFSYNC)" WORDSIZE_CHECK_CODE "" "" "" "" HAS_FCNTL_FSYNC)


if (NOT ESX)
    ##########################################
    # linux-aio probe

    set(LIBAIO_CHECK_CODE
        "
        #include <libaio.h>
        #include <stddef.h>
        int main(void)
        {
          io_setup(0, NULL);
          return 0;
        }
        "
    )
    compile_c_program_and_check("Linux AIO support" LIBAIO_CHECK_CODE "" "" "" "-laio" CONFIG_LIBAIO)
    if (CONFIG_LIBAIO)
        list(APPEND FIO_LIBRARIES aio)
    endif ()

    set(LIBAIO_RW_FLAGS_CHECK_CODE
        "
        #include <libaio.h>
        #include <stddef.h>
        int main(void)
        {
          io_prep_preadv2(NULL, 0, NULL, 0, 0, 0);
          io_prep_pwritev2(NULL, 0, NULL, 0, 0, 0);
          return 0;
        }
        "
    )
    compile_c_program_and_check("Linux AIO support rw flags" LIBAIO_RW_FLAGS_CHECK_CODE "" "" "" "-laio" CONFIG_LIBAIO_RW_FLAGS)
endif ()

##########################################
# posix aio probe

set(POSIX_AIO_CHECK_CODE
    "
    #include <aio.h>
    int main(void)
    {
      struct aiocb cb;
      aio_read(&cb);
      return 0;
    }
    "
)
compile_c_program_and_check("POSIX AIO support" POSIX_AIO_CHECK_CODE "" "" "" "" CONFIG_POSIXAIO)
if (CONFIG_POSIXAIO)
    set(CONFIG_POSIXAIO_FSYNC NO)
else ()
    compile_c_program_and_check("POSIX AIO support needs -lrt" POSIX_AIO_CHECK_CODE "" "" "" "-laio -lrt" CONFIG_POSIXAIO)
    if (CONFIG_POSIXAIO)
        set(CONFIG_POSIXAIO YES)
        list(APPEND FIO_LIBRARIES rt)
    endif ()
endif ()

##########################################
# posix aio fsync probe

if (CONFIG_POSIXAIO)
    set(POSIX_AIO_FSYNC_CHECK_CODE
        "
        #include <fcntl.h>
        #include <aio.h>
        int main(void)
        {
          struct aiocb cb;
          return aio_fsync(O_SYNC, &cb);
          return 0;
        }
        "
    )
    compile_c_program_and_check("POSIX AIO fsync" POSIX_AIO_FSYNC_CHECK_CODE "" "" "" "-laio" CONFIG_POSIXAIO_FSYNC)
else ()
    set(CONFIG_POSIXAIO_FSYNC NO)
endif ()

##########################################
# POSIX pshared attribute probe

set(POSIX_SHARED_CHECK_CODE
    "
    #include <unistd.h>
    int main(void)
    {
    #if defined(_POSIX_THREAD_PROCESS_SHARED) && ((_POSIX_THREAD_PROCESS_SHARED + 0) > 0)
    # if defined(__CYGWIN__)
    #  error \"_POSIX_THREAD_PROCESS_SHARED is buggy on Cygwin\"
    # elif defined(__APPLE__)
    #  include <AvailabilityMacros.h>
    #  include <TargetConditionals.h>
    #  if TARGET_OS_MAC && MAC_OS_X_VERSION_MIN_REQUIRED < 1070
    #   error \"_POSIX_THREAD_PROCESS_SHARED is buggy/unsupported prior to OSX 10.7\"
    #  endif
    # endif
    #else
    # error \"_POSIX_THREAD_PROCESS_SHARED is unsupported\"
    #endif
      return 0;
    }
    "
)
compile_c_program_and_check("POSIX pshared support" POSIX_SHARED_CHECK_CODE "" "" "" "" CONFIG_PSHARED)

##########################################
# POSIX pthread_condattr_setclock() probe

if (PTHREAD_CONDATTR_SETCLOCK)
    set(PTHREAD_CONDATTR_SETLOCK_CHECK_CODE
        "
        #include <pthread.h>
        int main(void)
        {
          pthread_condattr_t condattr;
          pthread_condattr_setclock(&condattr, CLOCK_MONOTONIC);
          return 0;
        }
        "
    )
    compile_c_program_and_check("pthread_condattr_setclock()" PTHREAD_CONDATTR_SETLOCK_CHECK_CODE "" "" "" "-lpthread" CONFIG_PTHREAD_CONDATTR_SETCLOCK)
    if (CONFIG_PTHREAD_CONDATTR_SETCLOCK)
        list(APPEND FIO_LIBRARIES Threads::Threads)
    endif ()
endif ()

##########################################
# pthread_sigmask() probe

set(PTHREAD_SIGMASK_CHECK_CODE
    "
    #include <stddef.h> /* NULL */
    #include <signal.h> /* pthread_sigmask() */
    int main(void)
    {
      return pthread_sigmask(0, NULL, NULL);
    }
    "
)
compile_c_program_and_check("pthread_sigmask()" PTHREAD_SIGMASK_CHECK_CODE "" "" "" "-lpthread" CONFIG_PTHREAD_SIGMASK)
if (CONFIG_PTHREAD_SIGMASK)
    list(APPEND FIO_LIBRARIES Threads::Threads)
endif ()

##########################################
# pthread_getaffinity_np() probe

set(PTHREAD_GETAFFINITY_CHECK_CODE
    "
    #include <stddef.h> /* NULL */
    #include <signal.h> /* pthread_sigmask() */
    #include <pthread.h>
    int main(void)
    {
      cpu_set_t set;
      return pthread_getaffinity_np(pthread_self(), sizeof(set), &set);
    }
    "
)
compile_c_program_and_check("pthread_getaffinity_np()" PTHREAD_GETAFFINITY_CHECK_CODE "" "" "" "-lpthread" CONFIG_PTHREAD_GETAFFINITY)
if (CONFIG_PTHREAD_GETAFFINITY)
    list(APPEND FIO_LIBRARIES Threads::Threads)
endif ()

##########################################
# solaris aio probe

set(PTHREAD_SOLARISAIO_CHECK_CODE
    "
    #include <sys/types.h>
    #include <sys/asynch.h>
    #include <unistd.h>
    int main(void)
    {
      aio_result_t res;
      return aioread(0, NULL, 0, 0, SEEK_SET, &res);
      return 0;
    }
    "
)
compile_c_program_and_check("Solaris AIO support" PTHREAD_SOLARISAIO_CHECK_CODE "" "" "" "-laio" CONFIG_SOLARISAIO)
if (CONFIG_SOLARISAIO)
    list(APPEND FIO_LIBRARIES aio)
endif ()

##########################################
# __sync_fetch_and_add test

set(SFAA_CHECK_CODE
    "
    #include <inttypes.h>
    static int sfaa(uint64_t *ptr)
    {
      return __sync_fetch_and_add(ptr, 0);
    }

    int main(int argc, char **argv)
    {
      uint64_t val = 42;
      sfaa(&val);
      return val;
    }
    "
)
compile_c_program_and_check("__sync_fetch_and_add" SFAA_CHECK_CODE "" "" "" "" CONFIG_SFAA)

##########################################
# __sync_synchronize() test

set(SYNC_SYNC_CHECK_CODE
    "
    #include <inttypes.h>

    int main(int argc, char **argv)
    {
      __sync_synchronize();
      return 0;
    }
    "
)
compile_c_program_and_check("__sync_synchronize" SYNC_SYNC_CHECK_CODE "" "" "" "" CONFIG_SYNC_SYNC)

##########################################
# __sync_val_compare_and_swap() test

set(CMP_SWAP_CHECK_CODE
    "
    #include <inttypes.h>

    int main(int argc, char **argv)
    {
      int x = 0;
      return __sync_val_compare_and_swap(&x, 1, 2);
    }
    "
)
compile_c_program_and_check("__sync_val_compare_and_swap" CMP_SWAP_CHECK_CODE "" "" "" "" CONFIG_CMP_SWAP)

if (NOT DISABLE_RDMA)
    ##########################################
    # libverbs probe

    set(IBVERBS_CHECK_CODE
        "
        #include <infiniband/verbs.h>
        int main(int argc, char **argv)
        {
          struct ibv_pd *pd = ibv_alloc_pd(NULL);
          return pd != NULL;
        }
        "
    )
    compile_c_program_and_check("ibverbs" IBVERBS_CHECK_CODE "" "" "" "-libverbs" HAS_IBVERBS)

    ##########################################
    # rdmacm probe

    set(RDMACM_CHECK_CODE
        "
        #include <stdio.h>
        #include <rdma/rdma_cma.h>
        int main(int argc, char **argv)
        {
          rdma_destroy_qp(NULL);
          return 0;
        }
        "
    )
    compile_c_program_and_check("rdmacm" RDMACM_CHECK_CODE "" "" "" "-lrdmacm" HAS_RDMACM)

    ##########################################
    # librpma probe
    # The librpma engines require librpma>=v0.11.0 with rpma_cq_get_wc().

    set(RPMA_CHECK_CODE
        "
        #include <librpma.h>
        int main(void)
        {
          void *ptr = rpma_cq_get_wc;
          (void) ptr; /* unused */
          return 0;
        }
        "
    )
    compile_c_program_and_check("rpma" RPMA_CHECK_CODE "" "" "" "-lrpma" HAS_RPMA)
else ()
    set(HAS_IBVERBS NO)
    message(STATUS "libverbs " ${HAS_IBVERBS})
    set(HAS_RDMACM NO)
    message(STATUS "rdmacm " ${HAS_RDMACM})
    set(HAS_RPMA NO)
    message(STATUS "librpma " ${HAS_RPMA})
endif ()

##########################################
# libprotobuf-c probe

set(PROTOBUF_C_CHECK_CODE
    "
    #include <stdio.h>
    #include <protobuf-c/protobuf-c.h>
    #if !defined(PROTOBUF_C_VERSION_NUMBER)
    # error PROTOBUF_C_VERSION_NUMBER is not defined!
    #endif
    int main(int argc, char **argv)
    {
      (void)protobuf_c_message_check(NULL);
      return 0;
    }
    "
)
compile_c_program_and_check("libprotobuf_c" PROTOBUF_C_CHECK_CODE "" "" "" "-lprotobuf-c" HAS_PROTOBUF_C)

##########################################
# asprintf() and vasprintf() probes

set(ASPRINTF_CHECK_CODE
    "
    #include <stdio.h>

    int main(int argc, char **argv)
    {
      char *buf;
      return asprintf(&buf, \"%s\", \"str\") == 0;
    }
    "
)
compile_c_program_and_check("asprintf()" ASPRINTF_CHECK_CODE "" "" "" "" CONFIG_HAVE_ASPRINTF)

set(VASPRINTF_CHECK_CODE
    "
    #include <stdio.h>

    int main(int argc, char **argv)
    {
      va_list ap;
      char *buf;
      return vasprintf(&buf, \"%s\", ap) == 0;
    }
    "
)
compile_c_program_and_check("vasprintf()" VASPRINTF_CHECK_CODE "" "" "" "" CONFIG_HAVE_VASPRINTF)

##########################################
# Linux fallocate probe

set(FALLOCATE_CHECK_CODE
    "
    #include <stdio.h>
    #include <fcntl.h>
    #include <linux/falloc.h>
    int main(int argc, char **argv)
    {
      int r = fallocate(0, FALLOC_FL_KEEP_SIZE, 0, 1024);
      return r;
    }
    "
)
compile_c_program_and_check("Linux fallocate" FALLOCATE_CHECK_CODE "" "" "" "" CONFIG_LINUX_FALLOCATE)

##########################################
# POSIX fadvise probe

set(POSIX_FADVISE_CHECK_CODE
    "
    #include <stdio.h>
    #include <fcntl.h>
    int main(int argc, char **argv)
    {
      int r = posix_fadvise(0, 0, 0, POSIX_FADV_NORMAL);
      return r;
    }
    "
)
compile_c_program_and_check("POSIX fadvise" POSIX_FADVISE_CHECK_CODE "" "" "" "" CONFIG_POSIX_FADVISE)

##########################################
# POSIX fallocate probe

set(POSIX_FALLOCATE_CHECK_CODE
    "
    #include <stdio.h>
    #include <fcntl.h>
    int main(int argc, char **argv)
    {
      int r = posix_fallocate(0, 0, 1024);
      return r;
    }
    "
)
compile_c_program_and_check("POSIX fallocate" POSIX_FALLOCATE_CHECK_CODE "" "" "" "" CONFIG_POSIX_FALLOCATE)

##########################################
# sched_set/getaffinity 2 or 3 argument test

set(LINUX_3ARG_AFFINITY_CHECK_CODE
    "
    #include <sched.h>
    int main(int argc, char **argv)
    {
      cpu_set_t mask = { };

      return sched_setaffinity(0, sizeof(mask), &mask);
    }
    "
)
compile_c_program_and_check("sched_setaffinity(3 arg)" LINUX_3ARG_AFFINITY_CHECK_CODE "" "" "" "" CONFIG_3ARG_AFFINITY)

if (CONFIG_3ARG_AFFINITY)
    set(CONFIG_2ARG_AFFINITY NO)
else ()
    set(LINUX_2ARG_AFFINITY_CHECK_CODE
        "
        #include <sched.h>
        int main(int argc, char **argv)
        {
          cpu_set_t mask = { };

          return sched_setaffinity(0, &mask);
        }
        "
    )
    compile_c_program_and_check("sched_setaffinity(2 arg)" LINUX_2ARG_AFFINITY_CHECK_CODE "" "" "" "" CONFIG_2ARG_AFFINITY)
endif ()

##########################################
# clock_gettime probe

set(CLOCK_GETTIME_CHECK_CODE
    "
    #include <stdio.h>
    #include <time.h>
    int main(int argc, char **argv)
    {
      struct timespec ts;

      return clock_gettime(0, &ts);
    }
    "
)
compile_c_program_and_check("clock_gettime" CLOCK_GETTIME_CHECK_CODE "" "" "" "" CONFIG_CLOCK_GETTIME)

if (NOT CONFIG_CLOCK_GETTIME)
    compile_c_program_and_check("clock_gettime" CLOCK_GETTIME_CHECK_CODE "" "" "" "-lrt" CONFIG_CLOCK_GETTIME)
    if (CONFIG_CLOCK_GETTIME)
        list(APPEND FIO_LIBRARIES rt)
    endif ()
endif ()

##########################################
# CLOCK_MONOTONIC probe

if (CONFIG_CLOCK_GETTIME)
    set(CLOCK_MONOTONIC_CHECK_CODE
        "
        #include <stdio.h>
        #include <time.h>
        int main(int argc, char **argv)
        {
          struct timespec ts;

          return clock_gettime(CLOCK_MONOTONIC, &ts);
        }
        "
    )
    compile_c_program_and_check("CLOCK_MONOTONIC" CLOCK_MONOTONIC_CHECK_CODE "" "" "" "" CONFIG_CLOCK_MONOTONIC)
else ()
    set(CONFIG_CLOCK_MONOTONIC NO)
    message(STATUS "CLOCK_MONOTONIC " ${CONFIG_CLOCK_MONOTONIC})
endif ()

##########################################
# clockid_t probe

set(CLOCKID_T_CHECK_CODE
    "
    #include <time.h>
    #include <string.h>
    int main(int argc, char **argv)
    {
      volatile clockid_t cid;
      memset((void*)&cid, 0, sizeof(cid));
      return 0;
    }
    "
)
compile_c_program_and_check("clockid_t" CLOCKID_T_CHECK_CODE "" "" "" "" CONFIG_CLOCKID_T)

##########################################
# gettimeofday() probe

set(GETTIMEOFDAY_CHECK_CODE
    "
    #include <sys/time.h>
    #include <stdio.h>
    int main(int argc, char **argv)
    {
      struct timeval tv;
      return gettimeofday(&tv, NULL);
    }
    "
)
compile_c_program_and_check("gettimeofday" GETTIMEOFDAY_CHECK_CODE "" "" "" "" CONFIG_GETTIMEOFDAY)

##########################################
# fdatasync() probe

set(FDATASYNC_CHECK_CODE
    "
    #include <stdio.h>
    #include <unistd.h>
    int main(int argc, char **argv)
    {
      return fdatasync(0);
    }
    "
)
compile_c_program_and_check("fdatasync" FDATASYNC_CHECK_CODE "" "" "" "" CONFIG_FDATASYNC)

##########################################
# pipe() probe

set(PIPE_CHECK_CODE
    "
    #include <unistd.h>
    int main(int argc, char **argv)
    {
      int fd[2];
      return pipe(fd);
    }
    "
)
compile_c_program_and_check("pipe()" PIPE_CHECK_CODE "" "" "" "" CONFIG_PIPE)

##########################################
# pipe2() probe

set(PIPE2_CHECK_CODE
    "
    #include <unistd.h>
    int main(int argc, char **argv)
    {
      int fd[2];
      return pipe2(fd, 0);
    }
    "
)
compile_c_program_and_check("pipe2()" PIPE2_CHECK_CODE "" "" "" "" CONFIG_PIPE2)

##########################################
# pread() probe

set(PREAD_CHECK_CODE
    "
    #include <unistd.h>
    int main(int argc, char **argv)
    {
      return pread(0, NULL, 0, 0);
    }
    "
)
compile_c_program_and_check("pread()" PREAD_CHECK_CODE "" "" "" "" CONFIG_PREAD)

##########################################
# sync_file_range() probe

set(SYNC_FILE_RANGE_CHECK_CODE
    "
    #include <stdio.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <linux/fs.h>
    int main(int argc, char **argv)
    {
      unsigned int flags = SYNC_FILE_RANGE_WAIT_BEFORE | SYNC_FILE_RANGE_WRITE |
                SYNC_FILE_RANGE_WAIT_AFTER;
      return sync_file_range(0, 0, 0, flags);
    }
    "
)
compile_c_program_and_check("sync_file_range" SYNC_FILE_RANGE_CHECK_CODE "" "" "" "" CONFIG_SYNC_FILE_RANGE)

# ##########################################
# # ASharedMemory_create() probe

set(ASHARED_MEMORY_CREATE_CHECK_CODE
    "
    #include <android/sharedmem.h>
    int main(int argc, char **argv)
    {
      return ASharedMemory_create(\"\", 0);
    }
    "
)
compile_c_program_and_check("ASharedMemory_create" ASHARED_MEMORY_CREATE_CHECK_CODE "" "" "" "" CONFIG_ASHAREDMEMORY_CREATE)


##########################################
# ext4 move extent probe

set(LINUX_EXT4_MOVE_EXTENT_CHECK_CODE
    "
    #include <fcntl.h>
    #include <sys/ioctl.h>
    int main(int argc, char **argv)
    {
      struct move_extent me;
      return ioctl(0, EXT4_IOC_MOVE_EXT, &me);
    }
    "
)
compile_c_program_and_check("EXT4 move extent" LINUX_EXT4_MOVE_EXTENT_CHECK_CODE "" "" "" "" CONFIG_LINUX_EXT4_MOVE_EXTENT)
if (NOT CONFIG_LINUX_EXT4_MOVE_EXTENT AND ${CONFIG_TARGET_OS} STREQUAL "Linux")
    # On Linux, just default to it on and let it error at runtime if we really
    # don't have it. None of my updated systems have it defined, but it does
    # work. Takes a while to bubble back.
    set(CONFIG_LINUX_EXT4_MOVE_EXTENT YES)
    message(STATUS "EXT4 move extent (Linux) " ${CONFIG_LINUX_EXT4_MOVE_EXTENT})
endif ()

##########################################
# splice probe

set(LINUX_SPLICE_CHECK_CODE
    "
    #include <stdio.h>
    #include <fcntl.h>
    int main(int argc, char **argv)
    {
      return splice(0, NULL, 0, NULL, 0, SPLICE_F_NONBLOCK);
    }
    "
)
compile_c_program_and_check("Linux splice(2)" LINUX_SPLICE_CHECK_CODE "" "" "" "" CONFIG_LINUX_SPLICE)

if (NOT DISABLE_NUMA)
    ##########################################
    # libnuma probe

    set(LIBNUMA_CHECK_CODE
        "
        #include <numa.h>
        int main(int argc, char **argv)
        {
          return numa_available();
        }
        "
    )
    compile_c_program_and_check("libnuma" LIBNUMA_CHECK_CODE "" "" "" "-lnuma" HAS_LIBNUMA)
    if (HAS_LIBNUMA)
        list(APPEND FIO_LIBRARIES numa)

        ##########################################
        # libnuma 2.x version API

        set(LIBNUMA2_CHECK_CODE
            "
            #include <numa.h>
            int main(int argc, char **argv)
            {
              struct bitmask *mask = numa_parse_nodestring(NULL);
              return mask->size == 0;
            }
            "
        )
        compile_c_program_and_check("libnuma v2" LIBNUMA2_CHECK_CODE "" "" "" "-lnuma" CONFIG_LIBNUMA)
    endif ()
else ()
    set(HAS_LIBNUMA NO)
    message(STATUS "libnuma " HAS_LIBNUMA)
endif ()

##########################################
# strsep() probe

set(STRSEP_CHECK_CODE
    "
    #include <string.h>
    int main(int argc, char **argv)
    {
      static char *string = \"This is a string\";
      strsep(&string, \"needle\");
      return 0;
    }
    "
)
compile_c_program_and_check("strsep" STRSEP_CHECK_CODE "" "" "" "" CONFIG_STRSEP)

##########################################
# strcasestr() probe

set(STRCASESTR_CHECK_CODE
    "
    #include <string.h>
    int main(int argc, char **argv)
    {
      return strcasestr(argv[0], argv[1]) != NULL;
    }
    "
)
compile_c_program_and_check("strcasestr" STRCASESTR_CHECK_CODE "" "" "" "" CONFIG_STRCASESTR)

##########################################
# strlcat() probe

set(STRLCAT_CHECK_CODE
    "
    #include <string.h>
    int main(int argc, char **argv)
    {
      static char dst[64];
      static char const *string = \"This is a string\";
      memset(dst, 0, sizeof(dst));
      strlcat(dst, string, sizeof(dst));
      return 0;
    }
    "
)
compile_c_program_and_check("strlcat" STRLCAT_CHECK_CODE "" "" "" "" CONFIG_STRLCAT)

##########################################
# getopt_long_only() probe

set(GETOPT_LONG_ONLY_CHECK_CODE
    "
    #include <unistd.h>
    #include <stdio.h>
    #include <getopt.h>
    int main(int argc, char **argv)
    {
      int c = getopt_long_only(argc, argv, \"\", NULL, NULL);
      return c;
    }
    "
)
compile_c_program_and_check("getopt_long_only()" GETOPT_LONG_ONLY_CHECK_CODE "" "" "" "" CONFIG_GETOPT_LONG_ONLY)

##########################################
# inet_aton() probe

set(INET_ATON_CHECK_CODE
    "
    #ifdef _WIN32
    #include <winsock2.h>
    #else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #endif
    #include <stdio.h>
    int main(int argc, char **argv)
    {
      struct in_addr in;
      return inet_aton(NULL, &in);
    }
    "
)
compile_c_program_and_check("inet_aton" INET_ATON_CHECK_CODE "" "" "" "" CONFIG_INET_ATON)

##########################################
# socklen_t probe

set(SOCKLEN_T_CHECK_CODE
    "
    #ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #else
    #include <sys/socket.h>
    #endif
    int main(int argc, char **argv)
    {
      socklen_t len = 0;
      return len;
    }
    "
)
compile_c_program_and_check("socklen_t" SOCKLEN_T_CHECK_CODE "" "" "" "" CONFIG_SOCKLEN_T)

##########################################
# Whether or not __thread is supported for TLS

set(TLS_THREAD_CHECK_CODE
    "
    #include <stdio.h>
    static __thread int ret;
    int main(int argc, char **argv)
    {
      return ret;
    }
    "
)
compile_c_program_and_check("__thread" TLS_THREAD_CHECK_CODE "" "" "" "" CONFIG_TLS_THREAD)

if (ENABLE_GFIO)
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(GTK2_0 glib-2.0>=glib-2.18 REQUIRED gtk+-2.0)
    pkg_check_modules(GTHREAD2_0 REQUIRED gthread-2.0)

    ##########################################
    # Check if we have required gtk/glib support for gfio

    set(GLIB_GTK_CAIRO_CHECK_CODE
        "
        #include <glib.h>
        #include <cairo.h>
        #include <gtk/gtk.h>
        int main(void)
        {
          gdk_threads_enter();
          gdk_threads_leave();

          return GTK_CHECK_VERSION(2, 18, 0) ? 0 : 1; /* 0 on success */
        }
        "
    )

    set(CHECK_CFLAGS ${GTK2_0_CFLAGS} ${GTHREAD2_0_CFLAGS})
    list(JOIN CHECK_CFLAGS " " CHECK_CFLAGS_STRING)

    set(CHECK_LIBRARIES ${GTK2_0_LIBRARIES} ${GTHREAD2_0_LIBRARIES})
    list(JOIN CHECK_LIBRARIES " -l" CHECK_LIBRARIES_STRING)
    string(PREPEND CHECK_LIBRARIES_STRING "-l")

    compile_c_program_and_check("gtk 2.18 or higher"
                      GLIB_GTK_CAIRO_CHECK_CODE
                      ""
                      ${CHECK_CFLAGS_STRING}
                      ""
                      ${CHECK_LIBRARIES_STRING}
                      CONFIG_GFIO)
    if (CONFIG_GFIO)
        # set(GFIO_INCLUDE_DIRS ${GTK2_0_INCLUDE_DIRS} ${GTHREAD2_0_INCLUDE_DIRS})
        set(GFIO_CFLAGS ${CHECK_CFLAGS})
        set(GFIO_LIBRARIES ${CHECK_LIBRARIES})
    endif ()
endif ()

##########################################
# Check whether we have getrusage(RUSAGE_THREAD)

set(RUSAGE_THREAD_CHECK_CODE
    "
    #include <sys/time.h>
    #include <sys/resource.h>
    int main(int argc, char **argv)
    {
      struct rusage ru;
      getrusage(RUSAGE_THREAD, &ru);
      return 0;
    }
    "
)
compile_c_program_and_check("RUSAGE_THREAD" RUSAGE_THREAD_CHECK_CODE "" "" "" "" CONFIG_RUSAGE_THREAD)

##########################################
# Check whether we have SCHED_IDLE

set(SCHED_IDLE_CHECK_CODE
    "
    #include <sched.h>
    int main(int argc, char **argv)
    {
      struct sched_param p = { };

      return sched_setscheduler(0, SCHED_IDLE, &p);
    }
    "
)
compile_c_program_and_check("SCHED_IDLE" SCHED_IDLE_CHECK_CODE "" "" "" "" CONFIG_SCHED_IDLE)

##########################################
# Check whether we have TCP_NODELAY

set(TCP_NODELAY_CHECK_CODE
    "
    #ifdef _WIN32
    #include <winsock2.h>
    #else
    #include <stdio.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/tcp.h>
    #endif
    int main(int argc, char **argv)
    {
      return getsockopt(0, 0, TCP_NODELAY, NULL, NULL);
    }
    "
)
compile_c_program_and_check("TCP_NODELAY" TCP_NODELAY_CHECK_CODE "" "" "" "" CONFIG_TCP_NODELAY)
if (NOT CONFIG_TCP_NODELAY)
    compile_c_program_and_check("TCP_NODELAY with -lws2_32" TCP_NODELAY_CHECK_CODE "" "" "" "-lws2_32" CONFIG_TCP_NODELAY)
    list(APPEND FIO_LIBRARIES ws2_32)
endif ()

##########################################
# Check whether we have SO_SNDBUF

set(WINDOW_SIZE_CHECK_CODE
    "
    #ifdef _WIN32
    #include <winsock2.h>
    #else
    #include <stdio.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/tcp.h>
    #endif
    int main(int argc, char **argv)
    {
      setsockopt(0, SOL_SOCKET, SO_SNDBUF, NULL, 0);
      setsockopt(0, SOL_SOCKET, SO_RCVBUF, NULL, 0);
    }
    "
)
compile_c_program_and_check("Net engine window_size" WINDOW_SIZE_CHECK_CODE "" "" "" "" CONFIG_NET_WINDOWSIZE)
if (NOT CONFIG_NET_WINDOWSIZE)
    compile_c_program_and_check("Net engine window_size with -lws2_32" WINDOW_SIZE_CHECK_CODE "" "" "" "-lws2_32" CONFIG_NET_WINDOWSIZE)
    if (CONFIG_NET_WINDOWSIZE)
        list(APPEND FIO_LIBRARIES ws2_32)
    endif ()
endif ()

##########################################
# Check whether we have TCP_MAXSEG

set(MSS_CHECK_CODE
    "
    #ifdef _WIN32
    #include <winsock2.h>
    #else
    #include <stdio.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/tcp.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #endif
    int main(int argc, char **argv)
    {
      return setsockopt(0, IPPROTO_TCP, TCP_MAXSEG, NULL, 0);
    }
    "
)
compile_c_program_and_check("TCP_MAXSEG" MSS_CHECK_CODE "" "" "" "" CONFIG_NET_MSS)
if (NOT CONFIG_NET_MSS)
    compile_c_program_and_check("TCP_MAXSEG with -lws2_32" MSS_CHECK_CODE "" "" "" "-lws2_32" CONFIG_NET_MSS)
    if (CONFIG_NET_MSS)
        list(APPEND FIO_LIBRARIES ws2_32)
    endif ()
endif ()

##########################################
# Check whether we have RLIMIT_MEMLOCK

set(RLIMIT_MEMLOCK_CHECK_CODE
    "
    #include <sys/time.h>
    #include <sys/resource.h>
    int main(int argc, char **argv)
    {
      struct rlimit rl;
      return getrlimit(RLIMIT_MEMLOCK, &rl);
    }
    "
)
compile_c_program_and_check("RLIMIT_MEMLOCK" RLIMIT_MEMLOCK_CHECK_CODE "" "" "" "" CONFIG_RLIMIT_MEMLOCK)

##########################################
# Check whether we have pwritev/preadv

set(PWRITEV_PREADV_CHECK_CODE
    "
    #include <stdio.h>
    #include <sys/uio.h>
    int main(int argc, char **argv)
    {
      struct iovec iov[1] = { };

      return pwritev(0, iov, 1, 0) + preadv(0, iov, 1, 0);
    }
    "
)
compile_c_program_and_check("pwritev/preadv" PWRITEV_PREADV_CHECK_CODE "" "" "" "" CONFIG_PWRITEV)

##########################################
# Check whether we have pwritev2/preadv2

set(PWRITEV2_PREADV2_CHECK_CODE
    "
    #include <stdio.h>
    #include <sys/uio.h>
    int main(int argc, char **argv)
    {
      struct iovec iov[1] = { };

      return pwritev2(0, iov, 1, 0, 0) + preadv2(0, iov, 1, 0, 0);
    }
    "
)
compile_c_program_and_check("pwritev2/preadv2" PWRITEV2_PREADV2_CHECK_CODE "" "" "" "" CONFIG_PWRITEV2)

##########################################
# Check whether we have the required functions for ipv6

# set(IPV6_CHECK_CODE
#     "
#     #ifdef _WIN32
#     #include <winsock2.h>
#     #include <ws2tcpip.h>
#     #else
#     #include <sys/types.h>
#     #include <sys/socket.h>
#     #include <netinet/in.h>
#     #include <netdb.h>
#     #endif
#     #include <stdio.h>
#     int main(int argc, char **argv)
#     {
#       struct addrinfo hints = { };
#       struct in6_addr addr = in6addr_any;
#       int ret;

#       ret = getaddrinfo(NULL, NULL, &hints, NULL);
#       freeaddrinfo(NULL);
#       printf(\"%s %d\\n\", gai_strerror(ret), addr.s6_addr[0]);

#       return 0;
#     }
#     "
# )

set(IPV6_CHECK_CODE
    "
    #ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netdb.h>
    #endif
    #include <stdio.h>
    int main(int argc, char **argv)
    {
      struct addrinfo hints = { };
      struct in6_addr addr = in6addr_any;
      int ret;

      ret = getaddrinfo(NULL, NULL, &hints, NULL);
      freeaddrinfo(NULL);
      printf(\"%s %d\", gai_strerror(ret), addr.s6_addr[0]);

      return 0;
    }
    "
)

compile_c_program_and_check("IPv6 helpers" IPV6_CHECK_CODE "" "" "" "" CONFIG_IPV6)

if (NOT DISABLE_HTTP)
    set(HTTP_LIBS -lcurl -lssl -lcrypto)
    ##########################################
    # check for http

    set(OPENSSL_GE_1_1_0_CHECK_CODE
        "
        #include <curl/curl.h>
        #include <openssl/hmac.h>

        int main(int argc, char **argv)
        {
          CURL *curl;
          HMAC_CTX *ctx;

          curl = curl_easy_init();
          curl_easy_cleanup(curl);

          ctx = HMAC_CTX_new();
          HMAC_CTX_reset(ctx);
          HMAC_CTX_free(ctx);
          return 0;
        }
        "
    )
    # check for openssl >= 1.1.0, which uses an opaque HMAC_CTX pointer
    compile_c_program_and_check("openssl >= 1.1.0" OPENSSL_GE_1_1_0_CHECK_CODE "" "" "" "${HTTP_LIBS}" HAS_OPENSSL_GE_1_1_0)
    if (HAS_OPENSSL_GE_1_1_0)
        set(CONFIG_HAVE_OPAQUE_HMAC_CTX YES)
        set(CONFIG_HTTP YES)
    else ()
        set(OPENSSL_LT_1_1_0_CHECK_CODE
            "
            #include <curl/curl.h>
            #include <openssl/hmac.h>

            int main(int argc, char **argv)
            {
              CURL *curl;
              HMAC_CTX ctx;

              curl = curl_easy_init();
              curl_easy_cleanup(curl);

              HMAC_CTX_init(&ctx);
              HMAC_CTX_cleanup(&ctx);
              return 0;
            }
            "
        )
        # openssl < 1.1.0 uses the HMAC_CTX type directly
        compile_c_program_and_check("openssl < 1.1.0" OPENSSL_LT_1_1_0_CHECK_CODE "" "" "" "${HTTP_LIBS}" HAS_OPENSSL_LT_1_1_0)
        set(CONFIG_HTTP ${HAS_OPENSSL_LT_1_1_0})
    endif ()
else ()
    set(CONFIG_HTTP NO)
endif ()

message(STATUS "HTTP engine " ${CONFIG_HTTP})

if (NOT DISABLE_RADOS)
    ##########################################
    # check for rados

    set(RADOS_CHECK_CODE
        "
        #include <rados/librados.h>

        int main(int argc, char **argv)
        {
          rados_t cluster;
          rados_ioctx_t io_ctx;
          const char cluster_name[] = \"ceph\";
          const char user_name[] = \"client.admin\";
          const char pool[] = \"rados\";

          /* The rados_create2 signature required was only introduced in ceph 0.65 */
          rados_create2(&cluster, cluster_name, user_name, 0);
          rados_ioctx_create(cluster, pool, &io_ctx);

          return 0;
        }
        "
    )
    compile_c_program_and_check("Rados engine" RADOS_CHECK_CODE "" "" "" "-lrados" CONFIG_RADOS)

    ##########################################
    # check for rbd

    set(RADOS_BDEV_CHECK_CODE
        "
        #include <rbd/librbd.h>

        int main(int argc, char **argv)
        {
          rados_t cluster;
          rados_ioctx_t io_ctx;
          const char cluster_name[] = \"ceph\";
          const char user_name[] = \"client.admin\";
          const char pool[] = \"rbd\";
          int major, minor, extra;

          rbd_version(&major, &minor, &extra);
          /* The rados_create2 signature required was only introduced in ceph 0.65 */
          rados_create2(&cluster, cluster_name, user_name, 0);
          rados_ioctx_create(cluster, pool, &io_ctx);

          return 0;
        }
        "
    )
    compile_c_program_and_check("Rados Block Device engine" RADOS_BDEV_CHECK_CODE "" "" "" "-lrbd -lrados" CONFIG_RBD)
else ()
    set(CONFIG_RADOS NO)
    message(STATUS "Rados engine" ${CONFIG_RADOS})
    set(CONFIG_RBD NO)
    message(STATUS "Rados Block Device engine" ${CONFIG_RBD})
endif ()

if (CONFIG_RBD)
    ##########################################
    # check for rbd_poll

    set(RADOS_BDEV_POLL_CHECK_CODE
        "
        #include <rbd/librbd.h>
        #include <sys/eventfd.h>

        int main(int argc, char **argv)
        {
          rbd_image_t image;
          rbd_completion_t comp;

          int fd = eventfd(0, EFD_NONBLOCK);
          rbd_set_image_notification(image, fd, EVENT_TYPE_EVENTFD);
          rbd_poll_io_events(image, comp, 1);

          return 0;
        }
        "
    )
    compile_c_program_and_check("rbd_poll" RADOS_BDEV_POLL_CHECK_CODE "" "" "" "-lrbd -lrados" CONFIG_RBD_POLL)

    ##########################################
    # check for rbd_invalidate_cache()

    set(RADOS_BDEV_INVAL_CHECK_CODE
        "
        #include <rbd/librbd.h>

        int main(int argc, char **argv)
        {
          rbd_image_t image;

          return rbd_invalidate_cache(image);
        }
        "
    )
    compile_c_program_and_check("rbd_invalidate_cache" RADOS_BDEV_INVAL_CHECK_CODE "" "" "" "-lrbd -lrados" CONFIG_RBD_INVAL)
else ()
    set(CONFIG_RBD_POLL NO)
    message(STATUS "rbd_poll " ${CONFIG_RBD_POLL})
    set(CONFIG_RBD_INVAL NO)
    message(STATUS "rbd_invalidate_cache " ${CONFIG_RBD_INVAL})
endif ()

##########################################
# Check whether we have setvbuf

set(SETVBUF_CHECK_CODE
    "
    #include <stdio.h>
    int main(int argc, char **argv)
    {
      FILE *f = NULL;
      char buf[80];
      setvbuf(f, buf, _IOFBF, sizeof(buf));
      return 0;
    }
    "
)
compile_c_program_and_check("setvbuf" SETVBUF_CHECK_CODE "" "" "" "" CONFIG_SETVBUF)

if (NOT DISABLE_GFAPI)
    ##########################################
    # check for gfapi

    set(GFAPI_CHECK_CODE
        "
        #include <glusterfs/api/glfs.h>

        int main(int argc, char **argv)
        {
          glfs_t *g = glfs_new(\"foo\");

          return 0;
        }
        "
    )
    compile_c_program_and_check("Gluster API engine" GFAPI_CHECK_CODE "" "" "" "-lgfapi -lglusterfs" CONFIG_GFAPI)
endif ()

if (CONFIG_GFAPI)
    ##########################################
    # check for gfapi fadvise support

    set(GF_FADVISE_CHECK_CODE
        "
        #include <glusterfs/api/glfs.h>

        int main(int argc, char **argv)
        {
          struct glfs_fd *fd;
          int ret = glfs_fadvise(fd, 0, 0, 1);

          return 0;
        }
        "
    )
    compile_c_program_and_check("Gluster API use fadvise" GF_FADVISE_CHECK_CODE "" "" "" "-lgfapi -lglusterfs" CONFIG_GF_FADVISE)

    ##########################################
    # check for newer gfapi

    set(GF_NEW_API_CHECK_CODE
        "
        #include <glusterfs/api/glfs.h>

        int main(int argc, char **argv)
        {
          return glfs_fsync(NULL, NULL, NULL) && glfs_ftruncate(NULL, 0, NULL, NULL);
        }
        "
    )
    compile_c_program_and_check("Gluster new API" GF_NEW_API_CHECK_CODE "" "" "" "-lgfapi -lglusterfs" CONFIG_GF_NEW_API)

    ##########################################
    # check for gfapi trim support

    set(GF_TRIM_CHECK_CODE
        "
        #include <glusterfs/api/glfs.h>

        int main(int argc, char **argv)
        {
          return glfs_discard_async(NULL, 0, 0);
        }
        "
    )
    compile_c_program_and_check("Gluster API trim support" GF_TRIM_CHECK_CODE "" "" "" "-lgfapi -lglusterfs" CONFIG_GF_TRIM)
endif ()

##########################################
# Check whether we have MTD

set(MTD_CHECK_CODE
    "
    #include <string.h>
    #include <mtd/mtd-user.h>
    #include <sys/ioctl.h>
    int main(int argc, char **argv)
    {
      struct mtd_write_req ops;
      struct mtd_info_user info;
      memset(&ops, 0, sizeof(ops));
      info.type = MTD_MLCNANDFLASH;
      return ioctl(0, MEMGETINFO, &info);
    }
    "
)
compile_c_program_and_check("MTD" MTD_CHECK_CODE "" "" "" "" CONFIG_MTD)

##########################################
# Check if we have lex/yacc available

set(YACC NO)
set(YACC_IS_BISON NO)
set(LEX NO)
set(CONFIG_ARITHMETIC NO)

if (NOT DISABLE_LEX)
    if (NOT ${CONFIG_TARGET_OS} STREQUAL "SunOS")
        HAS("lex" LEX)
        HAS("bison" BISON)
        if (BISON)
            set(YACC YES)
            set(YACC_IS_BISON YES)
        else ()
            HAS("yacc" YACC)
        endif ()
        if (YACC AND LEX)
            set(CONFIG_ARITHMETIC YES)
        endif ()
        if (CONFIG_ARITHMETIC)
            set(FLEX_CHECK_CODE
                "
                extern int yywrap(void);

                int main(int argc, char **argv)
                {
                  yywrap();
                  return 0;
                }
                "
            )
            compile_c_program_and_check("FLEX support" FLEX_CHECK_CODE "" "" "" "-lfl" HAS_FLEX)
            if (HAS_FLEX)
                list(APPEND FIO_LIBRARIES fl)
            else ()
                compile_c_program_and_check("LEX support" FLEX_CHECK_CODE "" "" "" "-ll" HAS_LEX)
                if (HAS_LEX)
                    list(APPEND FIO_LIBRARIES ll)
                else ()
                    set(CONFIG_ARITHMETIC NO)
                endif ()
            endif ()
        endif ()
    endif ()
endif ()

if (YACC_IS_BISON)
    set(CONFIG_YACC "bison -y")
else ()
    set(CONFIG_YACC "yacc")
endif ()

# Check if lex fails using -o
if (CONFIG_ARITHMETIC)
    if (NOT FORCE_NO_LEX_O)
        set(CONFIG_LEX_USE_O NO)
    else ()
        EXEC("lex -o lex.yy.c exp/expression-parser.l" CONFIG_LEX_USE_O)
    endif ()
endif ()

message(STATUS "lex/yacc for arithmetic " ${CONFIG_ARITHMETIC})

##########################################
# Check whether we have setmntent/getmntent

set(GETMNTENT_CHECK_CODE
    "
    #include <stdio.h>
    #include <mntent.h>
    int main(int argc, char **argv)
    {
      FILE *mtab = setmntent(NULL, \"r\");
      struct mntent *mnt = getmntent(mtab);
      endmntent(mtab);
      return mnt != NULL;
    }
    "
)
compile_c_program_and_check("getmntent" GETMNTENT_CHECK_CODE "" "" "" "" CONFIG_GETMNTENT)

##########################################
# Check whether we have getmntinfo
# These are originally added for BSDs, but may also work
# on other operating systems with getmntinfo(3).

# getmntinfo(3) for FreeBSD/DragonFlyBSD/OpenBSD.
# Note that NetBSD needs -Werror to catch warning as error.

set(GETMNTINFO_CHECK_CODE
    "
    #include <stdio.h>
    #include <sys/param.h>
    #include <sys/mount.h>
    int main(int argc, char **argv)
    {
      struct statfs *st;
      return getmntinfo(&st, MNT_NOWAIT);
    }
    "
)
compile_c_program_and_check("getmntinfo" GETMNTINFO_CHECK_CODE "" "-Werror" "" "" CONFIG_GETMNTINFO)

if (NOT CONFIG_GETMNTINFO)
    # getmntinfo(3) for NetBSD.

    set(GETMNTINFO_STATVFS_CHECK_CODE
        "
        #include <stdio.h>
        #include <sys/statvfs.h>
        int main(int argc, char **argv)
        {
          struct statvfs *st;
          return getmntinfo(&st, MNT_NOWAIT);
        }
        "
    )
    compile_c_program_and_check("getmntinfo_statvfs" GETMNTINFO_STATVFS_CHECK_CODE "" "-Werror" "" "" CONFIG_GETMNTINFO_STATVFS)
endif ()

##########################################
# Check whether we have _Static_assert

set(STATIC_ASSERT_CHECK_CODE
    "
    #include <assert.h>
    #include <stdlib.h>
    #include <stddef.h>

    struct foo {
      int a, b;
    };

    int main(int argc, char **argv)
    {
      _Static_assert(offsetof(struct foo, a) == 0 , \"Check\");
      return 0 ;
    }
    "
)
compile_c_program_and_check("Static Assert" STATIC_ASSERT_CHECK_CODE "" "" "" "" CONFIG_STATIC_ASSERT)

##########################################
# Check whether we have bool / stdbool.h

set(BOOL_CHECK_CODE
    "
    #include <stdbool.h>
    int main(int argc, char **argv)
    {
      bool var = true;
      return var != false;
    }
    "
)
compile_c_program_and_check("bool" BOOL_CHECK_CODE "" "" "" "" CONFIG_HAVE_BOOL)

##########################################
# Check whether we have strndup()

set(STRNDUP_CHECK_CODE
    "
    #include <string.h>
    #include <stdlib.h>
    int main(int argc, char **argv)
    {
      char *res = strndup(\"test string\", 8);

      free(res);
      return 0;
    }
    "
)
compile_c_program_and_check("strndup" STRNDUP_CHECK_CODE "" "" "" "" CONFIG_HAVE_STRNDUP)

##########################################
# <valgrind/drd.h> probe
# Note: presence of <valgrind/drd.h> implies that <valgrind/valgrind.h> is
# also available but not the other way around.

set(VALGRIND_DEV_CHECK_CODE
    "
    #include <valgrind/drd.h>
    int main(int argc, char **argv)
    {
      return 0;
    }
    "
)
compile_c_program_and_check("Valgrind headers" VALGRIND_DEV_CHECK_CODE "" "" "" "" CONFIG_VALGRIND_DEV)

if (${CONFIG_TARGET_OS} MATCHES "Linux|Android")
    ##########################################
    # <linux/blkzoned.h> probe

    set(LINUX_BLKZONED_CHECK_CODE
        "
        #include <linux/blkzoned.h>
        int main(int argc, char **argv)
        {
          return 0;
        }
        "
    )
    compile_c_program_and_check("Zoned block device support" LINUX_BLKZONED_CHECK_CODE "" "" "" "" CONFIG_HAS_BLKZONED)

    ##########################################
    # Check BLK_ZONE_REP_CAPACITY

    set(REP_CAPACITY_CHECK_CODE
        "
        #include <linux/blkzoned.h>
        int main(void)
        {
          return BLK_ZONE_REP_CAPACITY;
        }
        "
    )
    compile_c_program_and_check("Zoned block device capacity" REP_CAPACITY_CHECK_CODE "" "" "" "" CONFIG_HAVE_REP_CAPACITY)
endif ()

if (NOT DISABLE_LIBZBC)
    ##########################################
    # libzbc probe

    set(LIBZBC_CHECK_CODE
        "
        #include <libzbc/zbc.h>
        int main(int argc, char **argv)
        {
          struct zbc_device *dev = NULL;

          return zbc_open(\"foo=bar\", O_RDONLY, &dev);
        }
        "
    )

    if (EXISTS /usr/include/libzbc/libzbc)
        # SUSE Linux.
        set(DIR /usr/include/libzbc)
    endif()

    compile_c_program_and_check("libzbc engine" LIBZBC_CHECK_CODE "${DIR}" "" "" "-lzbc" CONFIG_LIBZBC)
    unset(DIR)

    # if ! check_min_lib_version libzbc 5; then
    #   libzbc="no"
    # fi
endif ()

if (${CONFIG_TARGET_OS} STREQUAL "Linux")
    ##########################################
    # Check NVME_URING_CMD support
    set(NVME_URING_CMD_CHECK_CODE
        "
        #include <linux/nvme_ioctl.h>
        int main(void)
        {
          return sizeof(struct nvme_uring_cmd);
        }
        "
    )
    compile_c_program_and_check("NVMe uring command support" NVME_URING_CMD_CHECK_CODE "" "" "" "" CONFIG_NVME_URING_CMD)
else ()
    set(CONFIG_NVME_URING_CMD NO)
    message(STATUS "NVMe uring command support " ${CONFIG_NVME_URING_CMD})
endif ()

##########################################
# Check if we have xnvme

if (NOT DISABLE_XNVME)
    # if check_min_lib_version xnvme 0.7.0; then
    #   xnvme="yes"
    #   xnvme_cflags=$(pkg-config --cflags xnvme)
    #   xnvme_libs=$(pkg-config --libs xnvme)
    # else
    #   xnvme="no"
    # fi
else ()
    set(CONFIG_LIBXNVME NO)
    message(STATUS "xnvme engine " ${CONFIG_LIBXNVME})
endif ()

##########################################
# Check if we have libblkio

if (NOT DISABLE_LIBBLKIO)
    # if check_min_lib_version blkio 1.0.0; then
    #   libblkio="yes"
    #   libblkio_cflags=$(pkg-config --cflags blkio)
    #   libblkio_libs=$(pkg-config --libs blkio)
    # else
    #   if test "$libblkio" = "yes" ; then
    #     feature_not_found "libblkio" "libblkio-dev or libblkio-devel"
    #   fi
    #   libblkio="no"
    # fi
else ()
    set(CONFIG_LIBBLKIO NO)
    message(STATUS "libblkio engine " ${CONFIG_LIBBLKIO})
endif ()

# ##########################################
# # check march=armv8-a+crc+crypto
# march_armv8_a_crc_crypto="no"
# if test "$cpu" = "arm64" ; then
#   cat > $TMPC <<EOF
# #if __linux__
# #include <arm_acle.h>
# #include <arm_neon.h>
# #include <sys/auxv.h>
# #endif

# int main(void)
# {
#   /* Can we also do a runtime probe? */
# #if __linux__
#   return getauxval(AT_HWCAP);
# #elif defined(__APPLE__)
#   return 0;
# #else
# # error "Don't know how to do runtime probe for ARM CRC32c"
# #endif
# }
# EOF
#   if compile_prog "-march=armv8-a+crc+crypto" "" "ARM CRC32c"; then
#     march_armv8_a_crc_crypto="yes"
#     CFLAGS="$CFLAGS -march=armv8-a+crc+crypto"
#     march_set="yes"
#   fi
# fi
# print_config "march_armv8_a_crc_crypto" "$march_armv8_a_crc_crypto"


##########################################
# cuda probe

set(CUDA_CHECK_CODE
    "
    #include <cuda.h>
    int main(int argc, char **argv)
    {
      return cuInit(0);
    }
    "
)
compile_c_program_and_check("cuda" CUDA_CHECK_CODE "" "" "" "-lcuda" CONFIG_CUDA)
if (CONFIG_CUDA)
    list(APPEND FIO_LIBRARIES cuda)
elseif (ENABLE_CUDA)
    message(FATAL_ERROR "cuda required but not found")
endif ()

##########################################
# libcufile probe

set(LIBCUFILE_CHECK_CODE
    "
    #include <cufile.h>

    int main(int argc, char* argv[]) {
       cuFileDriverOpen();
       return 0;
    }
    "
)
compile_c_program_and_check("libcufile" LIBCUFILE_CHECK_CODE "" "" "" "-lcuda -lcudart -lcufile -ldl" CONFIG_LIBCUFILE)
if (CONFIG_LIBCUFILE)
    list(APPEND FIO_LIBRARIES cuda cudart cufile dl)
elseif (ENABLE_LIBCUFILE)
    message(FATAL_ERROR "libcufile required but not found")
endif ()

##########################################
# check for cc -march=native

if (NOT DISABLE_NATIVE AND NOT DISABLE_OPTIMIZATIONS)
    set(MARCH_NATIVE_CHECK_CODE
        "
        int main(int argc, char **argv)
        {
          return 0;
        }
        "
    )
    compile_c_program_and_check("Build march=native" MARCH_NATIVE_CHECK_CODE "" "-march=native" "" "" CONFIG_BUILD_NATIVE)
else ()
    set(CONFIG_BUILD_NATIVE NO)
    message(STATUS "Build march=native " ${CONFIG_BUILD_NATIVE})
endif ()

##########################################
# check for -lcunit

set(CUNIT_CHECK_CODE
    "
    #include <CUnit/CUnit.h>
    #include <CUnit/Basic.h>
    int main(void)
    {
      if (CU_initialize_registry() != CUE_SUCCESS)
        return CU_get_error();
      CU_basic_set_mode(CU_BRM_VERBOSE);
      CU_basic_run_tests();
      CU_cleanup_registry();
      return CU_get_error();
    }
    "
)
compile_c_program_and_check("CUnit" CUNIT_CHECK_CODE "" "" "" "-lcunit" CONFIG_HAVE_CUNIT)

##########################################
# check for __kernel_rwf_t

set(KERNEL_RWF_T_CHECK_CODE
    "
    #include <linux/fs.h>
    int main(int argc, char **argv)
    {
      __kernel_rwf_t x;
      x = 0;
      return x;
    }
    "
)
compile_c_program_and_check("__kernel_rwf_t" KERNEL_RWF_T_CHECK_CODE "" "" "" "" CONFIG_HAVE_KERNEL_RWF_T)

##########################################
# check if gcc has -Wimplicit-fallthrough=2

set(FALLTHROUGH_CHECK_CODE
    "
    int main(int argc, char **argv)
    {
      return 0;
    }
    "
)
compile_c_program_and_check("-Wimplicit-fallthrough=2" FALLTHROUGH_CHECK_CODE "" "-Wimplicit-fallthrough=2" "" "" HAS_FALLTHROUGH)
if (HAS_FALLTHROUGH)
    add_compile_options(-Wimplicit-fallthrough)
endif ()

##########################################
# check if the compiler has -Wno-stringop-concatenation

set(NO_STRINGOP_CHECK_CODE
    "
    #include <stdio.h>

    int main(int argc, char **argv)
    {
        return printf(\"%s\", argv[0]);
    }
    "
)
compile_c_program_and_check("-Wno-stringop-truncation" NO_STRINGOP_CHECK_CODE "" "-Wno-stringop-truncation -Werror" "" "" CONFIG_HAVE_NO_STRINGOP)

if (NOT ESX)
    ##########################################
    # check for MADV_HUGEPAGE support

    set(THP_CHECK_CODE
        "
        #include <sys/mman.h>
        int main(void)
        {
          return madvise(0, 0x1000, MADV_HUGEPAGE);
        }
        "
    )
    compile_c_program_and_check("MADV_HUGEPAGE" THP_CHECK_CODE "" "" "" "" CONFIG_HAVE_THP)
else ()
    set(CONFIG_HAVE_THP NO)
    message(STATUS "MADV_HUGEPAGE " ${CONFIG_HAVE_THP})
endif ()

##########################################
# check for gettid()

set(GETTID_CHECK_CODE
    "
    #include <unistd.h>
    int main(int argc, char **argv)
    {
      return gettid();
    }
    "
)
compile_c_program_and_check("gettid" GETTID_CHECK_CODE "" "" "" "" CONFIG_HAVE_GETTID)

##########################################
# check for statx(2) support by libc

set(STATX_CHECK_CODE
    "
    #include <unistd.h>
    #include <sys/stat.h>

    int main(int argc, char **argv)
    {
        struct statx st;
        return statx(-1, *argv, 0, 0, &st);
    }
    "
)
compile_c_program_and_check("statx(2)/libc" STATX_CHECK_CODE "" "" "" "" CONFIG_HAVE_STATX)

##########################################
# check for statx(2) support by kernel

set(STATX_SYSCALL_CHECK_CODE
    "
    #include <unistd.h>
    #include <linux/stat.h>
    #include <sys/stat.h>
    #include <sys/syscall.h>

    static int _statx(int dfd, const char *pathname, int flags, unsigned int mask,
              struct statx *buffer)
    {
        return syscall(__NR_statx, dfd, pathname, flags, mask, buffer);
    }

    int main(int argc, char **argv)
    {
        struct statx st;
        return _statx(-1, *argv, 0, 0, &st);
    }
    "
)
compile_c_program_and_check("statx(2)/syscall" STATX_SYSCALL_CHECK_CODE "" "" "" "" CONFIG_HAVE_STATX_SYSCALL)

# ##########################################
# # check for Windows PDB generation support
# if test "pdb" != "no" ; then
#   cat > $TMPC <<EOF
# int main(void)
# {
#   return 0;
# }
# EOF
#   if compile_prog "-g -gcodeview" "-fuse-ld=lld -Wl,-pdb,$TMPO" "pdb"; then
#     pdb=yes
#   else
#     if test "$pdb" = "yes"; then
#       feature_not_found "PDB" "clang and lld"
#     fi
#     pdb=no
#   fi
# else
#   pdb=no
# fi
# print_config "Windows PDB generation" "$pdb"

##########################################
# check for timerfd support

set(TIMERFD_CREATE_CHECK_CODE
    "
    #include <sys/time.h>
    #include <sys/timerfd.h>

    int main(int argc, char **argv)
    {
        return timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    }
    "
)
compile_c_program_and_check("timerfd_create" TIMERFD_CREATE_CHECK_CODE "" "" "" "" CONFIG_HAVE_TIMERFD_CREATE)

if (CONFIG_LIBHDFS)
    list(APPEND FIO_SOURCES engines/libhdfs.c)
endif ()

if (CONFIG_LIBISCSI)
    list(APPEND FIO_SOURCES engines/libiscsi.c)
    list(APPEND ENGINES libiscsi)
endif ()

if (CONFIG_LIBNBD)
    list(APPEND FIO_SOURCES engines/nbd.c)
    list(APPEND ENGINES nbd)
endif ()

if (CONFIG_LIBNFS)
    list(APPEND FIO_SOURCES engines/nfs.c)
endif ()

if (CONFIG_LIBAIO)
    list(APPEND FIO_SOURCES
         engines/libaio.c
         engines/cmdprio.c)
    list(APPEND FIO_LIBRARIES aio)
    list(APPEND ENGINES libaio)
endif ()

if (CONFIG_RDMA)
    list(APPEND FIO_SOURCES engines/rdma.c)
    list(APPEND FIO_LIBRARIES ibverbs rdmacm)
    list(APPEND ENGINES rdma)
endif ()

if (CONFIG_LIBRPMA_APM)
    list(APPEND FIO_SOURCES
         engines/librpma_apm.c
         engines/librpma_fio.c)
    list(APPEND FIO_LIBRARIES rpma)
    if (CONFIG_LIBPMEM2_INSTALLED)
        list(APPEND FIO_LIBRARIES pmem2)
    else ()
        list(APPEND FIO_LIBRARIES pmem)
    endif ()
    list(APPEND ENGINES librpma_apm)
endif ()

if (CONFIG_LIBRPMA_GPSPM)
    list(APPEND FIO_SOURCES
         engines/librpma_gpspm.c
         engines/librpma_gpspm_flush.pb-c.c
         engines/librpma_fio.c)
    list(APPEND FIO_LIBRARIES rpma protobuf-c)
    if (${CONFIG_LIBPMEM2_INSTALLED})
        list(APPEND FIO_LIBRARIES pmem2)
    else ()
        list(APPEND FIO_LIBRARIES pmem)
    endif ()
    list(APPEND ENGINES librpma_gpspm)
endif ()

if (CONFIG_POSIXAIO)
    list(APPEND FIO_SOURCES engines/posixaio.c)
endif ()

if (CONFIG_LINUX_FALLOCATE)
    list(APPEND FIO_SOURCES engines/falloc.c)
endif ()

if (CONFIG_LINUX_EXT4_MOVE_EXTENT)
    list(APPEND FIO_SOURCES engines/e4defrag.c)
endif ()

if (CONFIG_LIBCUFILE)
    list(APPEND FIO_SOURCES engines/libcufile.c)
endif ()

if (CONFIG_LINUX_SPLICE)
    list(APPEND FIO_SOURCES engines/splice.c)
endif ()

if (CONFIG_SOLARISAIO)
    list(APPEND FIO_SOURCES engines/solarisaio.c)
endif ()

if (CONFIG_WINDOWSAIO)
    list(APPEND FIO_SOURCES engines/windowsaio.c)
endif ()

if (CONFIG_RADOS)
    list(APPEND FIO_SOURCES engines/rados.c)
    list(APPEND FIO_LIBRARIES rados)
    list(APPEND ENGINES rados)
endif ()

if (CONFIG_RBD)
    list(APPEND FIO_SOURCES engines/rbd.c)
    list(APPEND FIO_LIBRARIES rbd rados)
    list(APPEND ENGINES rbd)
endif ()

if (CONFIG_HTTP)
    list(APPEND FIO_SOURCES engines/http.c)
    list(APPEND FIO_LIBRARIES curl ssl crypto)
    list(APPEND ENGINES http)
endif ()

if (CONFIG_DFS)
    list(APPEND FIO_SOURCES engines/dfs.c)
    list(APPEND FIO_LIBRARIES uuid daos dfs)
    list(APPEND ENGINES http)
endif ()

list(APPEND FIO_SOURCES oslib/asprintf.cpp)

if (NOT CONFIG_STRSEP)
  list(APPEND FIO_SOURCES oslib/strsep.cpp)
endif ()

if (NOT CONFIG_STRCASESTR)
  list(APPEND FIO_SOURCES oslib/strcasestr.cpp)
endif ()

if (NOT CONFIG_STRLCAT)
  list(APPEND FIO_SOURCES oslib/strlcat.cpp)
endif ()

if (NOT CONFIG_HAVE_STRNDUP)
  list(APPEND FIO_SOURCES oslib/strndup.cpp)
endif ()

if (NOT CONFIG_GETOPT_LONG_ONLY)
  list(APPEND FIO_SOURCES oslib/getopt_long.cpp)
endif ()

if (NOT CONFIG_INET_ATON)
  list(APPEND FIO_SOURCES oslib/inet_aton.cpp)
endif ()

if (NOT CONFIG_HAVE_STATX)
  list(APPEND FIO_SOURCES oslib/statx.cpp)
endif ()

if (CONFIG_GFAPI)
    list(APPEND FIO_SOURCES
         engines/glusterfs.c
         engines/glusterfs_sync.c
         engines/glusterfs_async.c)
    list(APPEND FIO_LIBRARIES gfapi glusterfs)
    if (CONFIG_GF_FADVISE)
    endif ()
endif ()

if (CONFIG_MTD)
    list(APPEND FIO_SOURCES engines/mtd.c oslib/libmtd.cpp oslib/libmtd_legacy.cpp)
endif ()

if (CONFIG_LINUX_DEVDAX)
    list(APPEND FIO_SOURCES engines/dev-dax.c)
    list(APPEND FIO_LIBRARIES pmem)
    list(APPEND ENGINES dev-dax)
endif ()

if (CONFIG_LIBPMEM)
    list(APPEND FIO_SOURCES engines/libpmem.c)
    list(APPEND FIO_LIBRARIES pmem)
    list(APPEND ENGINES libpmem)
endif ()

if (CONFIG_IME)
    list(APPEND FIO_SOURCES engines/ime.c)
endif ()

if (CONFIG_LIBZBC)
    list(APPEND FIO_SOURCES engines/libzbc.c)
    list(APPEND FIO_LIBRARIES zbc)
    list(APPEND ENGINES libzbc)
endif ()

if (CONFIG_LIBXNVME)
    list(APPEND FIO_SOURCES engines/xnvme.c)
    list(APPEND FIO_LIBRARIES ${LIBXNVME_LIBS})
    list(APPEND ENGINES xnvme)
endif ()

if (CONFIG_LIBBLKIO)
    list(APPEND FIO_SOURCES engines/libblkio.c)
    list(APPEND FIO_LIBRARIES ${LIBBLKIO_LIBS})
    list(APPEND ENGINES libblkio)
endif ()

if (${CONFIG_TARGET_OS} MATCHES "Linux|Android")
    list(APPEND FIO_SOURCES
         diskutil.cpp
         fifo.cpp
         blktrace.cpp
         cgroup.cpp
         trim.cpp
         engines/sg.c
         oslib/linux-dev-lookup.cpp
         engines/io_uring.c
         engines/nvme.c
         engines/cmdprio.c)

    if (CONFIG_HAS_BLKZONED)
        list(APPEND FIO_SOURCES oslib/linux-blkzoned.cpp)
    endif ()

    list(APPEND FIO_LIBRARIES ${CMAKE_DL_LIBS})
    add_link_options(-rdynamic)
endif ()

if (${CONFIG_TARGET_OS} STREQUAL "Linux")
    list(APPEND FIO_LIBRARIES Threads::Threads)
endif ()

if (${CONFIG_TARGET_OS} STREQUAL "Android")
    list(APPEND FIO_SOURCES profiles/tiobench.cpp)
    list(APPEND FIO_LIBRARIES log)
endif ()

if (${CONFIG_TARGET_OS} STREQUAL "SunOS")
    list(APPEND FIO_LIBRARIES
         Threads::Threads
         ${CMAKE_DL_LIBS}
        )
    add_compile_definitions(__EXTENSIONS__)
endif ()

if (${CONFIG_TARGET_OS} STREQUAL "FreeBSD")
    list(APPEND FIO_SOURCES trim.cpp)
    list(APPEND FIO_LIBRARIES
         Threads::Threads
         ${CMAKE_DL_LIBS}
        )
    add_link_options(-rdynamic)
endif ()

if (${CONFIG_TARGET_OS} STREQUAL "OpenBSD")
    list(APPEND FIO_LIBRARIES Threads::Threads)
    add_link_options(-rdynamic)
endif ()

if (${CONFIG_TARGET_OS} STREQUAL "NetBSD")
    list(APPEND FIO_LIBRARIES
         Threads::Threads
         rt)
    add_link_options(-rdynamic)
endif ()

if (${CONFIG_TARGET_OS} STREQUAL "DragonFly")
    list(APPEND FIO_SOURCES trim.cpp)
    list(APPEND FIO_LIBRARIES
         Threads::Threads
         rt)
    add_link_options(-rdynamic)
endif ()

if (${CONFIG_TARGET_OS} STREQUAL "AIX")
    list(APPEND FIO_LIBRARIES
         Threads::Threads
         ${CMAKE_DL_LIBS}
         rt)
    add_compile_definitions(_LARGE_FILES __ppc__)
    add_link_options(-L/opt/freeware/lib
                     -Wl,-blibpath:/opt/freeware/lib:/usr/lib:/lib
                     -Wl,-bmaxdata:0x80000000)
endif ()

if (${CONFIG_TARGET_OS} STREQUAL "HP-UX")
    list(APPEND FIO_LIBRARIES
         Threads::Threads
         ${CMAKE_DL_LIBS}
         rt)
    add_compile_definition(_LARGEFILE64_SOURCE _XOPEN_SOURCE_EXTENDED)
endif ()

if (${CONFIG_TARGET_OS} STREQUAL "Darwin")
    list(APPEND FIO_LIBRARIES
         Threads::Threads
         ${CMAKE_DL_LIBS})
endif ()

if (${CONFIG_TARGET_OS} STREQUAL "CYGWIN")
    list(APPEND FIO_SOURCES
         os/windows/cpu-affinity.c
         os/windows/posix.c
         os/windows/dlls.c)
    # WINDOWS_OBJS = os/windows/cpu-affinity.o os/windows/posix.o os/windows/dlls.o lib/hweight.o
    list(APPEND FIO_LIBRARIES
         Threads::Threads
         psapi
         ws2_32
         ssp)
    add_compile_definitions(PSAPI_VERSION=1)
    include_directories(os/windows/posix/include)
    add_compile_options(-Wno-format)
endif ()

message(STATUS "seed_buckets: " ${SEED_BUCKETS})

if (HAS_IBVERBS AND HAS_RDMACM)
    set(CONFIG_RDMA YES)
endif ()

if ("${CPU}" MATCHES "x86_64" AND HAS_IBVERBS AND HAS_RDMACM AND HAS_RPMA AND (HAS_PMEM OR HAS_PMEM2))
    set(CONFIG_LIBRPMA_APM YES)
endif ()

if ("${CPU}" MATCHES "x86_64" AND HAS_IBVERBS AND HAS_RDMACM AND HAS_RPMA AND HAS_PROTOBUF_C AND (HAS_PMEM OR HAS_PMEM2))
    set(CONFIG_LIBRPMA_GPSPM YES)
endif ()

configure_file(${CMAKE_CURRENT_LIST_DIR}/cmake/config-host.h.in ${CMAKE_BINARY_DIR}/config-host.h @ONLY)

list(REMOVE_DUPLICATES FIO_SOURCES)
list(REMOVE_DUPLICATES FIO_LIBRARIES)

add_compile_options(
    -include ${CMAKE_BINARY_DIR}/config-host.h
    $<$<BOOL:${CONFIG_FIO_NO_OPT}>:-O3>
    $<$<BOOL:${CONFIG_BUILD_NATIVE}>:-march=native>
    $<$<BOOL:${CONFIG_PDB}>:-gcodeview>
    $<$<BOOL:$<C_COMPILER_ID:clang>>:-fno-builtin-stpcpy>
)

add_link_options(
    $<$<BOOL:${CONFIG_PDB}>:-fuse-ld=lld>
)

add_executable(fio fio.cpp ${FIO_SOURCES})
target_link_libraries(fio PRIVATE ${FIO_LIBRARIES})
install(TARGETS fio RUNTIME DESTINATION bin)

if (CONFIG_GFIO)
    set(GFIO_SOURCES
        graph.cpp
        tickmarks.cpp
        ghelpers.cpp
        goptions.cpp
        gerror.cpp
        gclient.cpp
        gcompat.cpp
        cairo_text_helpers.cpp
        printing.cpp
    )
    list(APPEND GFIO_SOURCES ${FIO_SOURCES})
    list(APPEND GFIO_LIBRARIES ${FIO_LIBRARIES})

    add_executable(gfio gfio.cpp ${GFIO_SOURCES})
    # target_include_directories(gfio PRIVATE ${GFIO_INCLUDE_DIRS})
    target_compile_options(gfio PRIVATE ${GFIO_CFLAGS})
    target_link_libraries(gfio PRIVATE ${GFIO_LIBRARIES})
    install(TARGETS gfio RUNTIME DESTINATION bin)
endif ()

install(FILES
        fio.1
        tools/fio_generate_plots.1
        tools/plot/fio2gnuplot.1
        tools/hist/fiologparser_hist.py.1
        DESTINATION share/man/man1)
