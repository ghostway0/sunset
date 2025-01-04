if DEBUG_BUILD
AM_CFLAGS += -DDEBUG
endif

bin_PROGRAMS = sunset tests json_test

sunset_SOURCES = main.c \
                 errors.c \
                 fonts.c \
                 ring_buffer.c \
                 geometry.c \
                 camera.c \
                 input.c \
                 events.c \
                 json.c \
                 bitmap.c \
                 shader.c \
                 ecs.c \
                 base64.c \
                 backend.c \
                 octree.c \
                 ui.c \
                 engine.c \
                 commands.c \
                 tga.c \
                 filesystem.c \
                 byte_stream.c \
                 mtl_file.c \
                 io.c \
                 obj_file.c \
                 images.c \
                 utils.c
sunset_CFLAGS = $(AM_CFLAGS)
sunset_CFLAGS = $(CFLAGS_COMMON)
sunset_LDADD = ../external/liblogc.la ../external/libcglm.la $(LIBS)

tests_SOURCES = tests.c \
                $(sunset_SOURCES) # Include sunset sources for testing
test_CFLAGS = $(AM_CFLAGS) -DHAVE_CMOCKA
test_CFLAGS = $(CFLAGS_COMMON)
test_LDADD = ../external/libsunset.la ../external/libcglm.la $(LIBS)
check_PROGRAMS = tests
check_DATA =
TESTS = tests

json_test_SOURCES = json_test.c \
                    $(sunset_SOURCES)
json_test_CFLAGS = $(AM_CFLAGS) -DHAVE_CMOCKA
json_test_CFLAGS = $(CFLAGS_COMMON)
json_test_LDADD = ../external/libsunset.la $(LIBS)
EXTRA_PROGRAMS = json_test

# Install the sunset executable
bindir = $(exec_prefix)/bin
bin_PROGRAMS += sunset
