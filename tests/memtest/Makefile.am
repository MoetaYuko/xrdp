
AM_CPPFLAGS = \
  -I$(top_srcdir)/common

PACKAGE_STRING = "memtest"

check_PROGRAMS = \
  memtest

memtest_SOURCES = \
  libmem.h \
  libmem.c \
  memtest.c

memtest_LDADD = \
  $(top_builddir)/common/libcommon.la

TESTS = \
  memtest
