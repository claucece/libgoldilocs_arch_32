include $(top_srcdir)/variables.am

check_PROGRAMS = test test_bench

test_SOURCES = test_goldilocks.cxx
test_CXXFLAGS = $(AM_CXXFLAGS) $(LANGXXFLAGS) $(WARNFLAGS) $(INCFLAGS) $(OFLAGS) $(ARCHFLAGS) $(GENFLAGS) $(XCXXFLAGS) $(LIBGOLDILOCKS_CXXFLAGS)
test_LDFLAGS = $(AM_LDFLAGS) $(XLDFLAGS) $(LIBGOLDILOCKS_LIBS)
test_LDADD = $(top_srcdir)/src/libgoldilocks.la

test_bench_SOURCES = bench_goldilocks.cxx
test_bench_CXXFLAGS = $(AM_CXXFLAGS) $(LANGXXFLAGS) $(WARNFLAGS) $(INCFLAGS) $(OFLAGS) $(ARCHFLAGS) $(GENFLAGS) $(XCXXFLAGS) $(LIBGOLDILOCKS_CXXFLAGS)
test_bench_LDFLAGS = $(AM_LDFLAGS) $(XLDFLAGS) $(LIBGOLDILOCKS_LIBS)
test_bench_LDADD = $(top_srcdir)/src/libgoldilocks.la
