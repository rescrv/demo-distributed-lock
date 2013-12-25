CC       = gcc
CXX      = g++
CFLAGS   = -pedantic -Wabi -Waddress -Wall -Warray-bounds -Wcast-align -Wcast-qual -Wchar-subscripts -Wclobbered -Wcomment -Wdisabled-optimization -Wempty-body -Wenum-compare -Wextra -Wfloat-equal -Wformat=2 -Wformat-nonliteral -Wformat-security -Wformat -Wformat-y2k -Wframe-larger-than=8192 -Wignored-qualifiers -Winit-self -Winline -Wlarger-than=4096 -Wlogical-op -Wmain -Wmissing-braces -Wmissing-field-initializers -Wmissing-format-attribute -Wmissing-include-dirs -Wno-long-long -Woverlength-strings -Wpacked-bitfield-compat -Wpacked -Wparentheses -Wpointer-arith -Wredundant-decls -Wreturn-type -Wsequence-point -Wshadow -Wsign-compare -Wstack-protector -Wstrict-aliasing=3 -Wstrict-aliasing -Wswitch-default -Wswitch-enum -Wswitch -Wtrigraphs -Wtype-limits -Wundef -Wuninitialized -Wunsafe-loop-optimizations -Wunused-function -Wunused-label -Wunused-parameter -Wunused-value -Wunused-variable -Wunused -Wvolatile-register-var -Wwrite-strings  -pedantic -Wabi -Waddress -Wall -Warray-bounds -Wcast-align -Wcast-qual -Wchar-subscripts -Wclobbered -Wcomment -Wdisabled-optimization -Wempty-body -Wenum-compare -Wextra -Wfloat-equal -Wformat=2 -Wformat-nonliteral -Wformat-security -Wformat -Wformat-y2k -Wframe-larger-than=8192 -Wignored-qualifiers -Wimplicit
CXXFLAGS = -pedantic -Wabi -Waddress -Wall -Warray-bounds -Wcast-align -Wcast-qual -Wchar-subscripts -Wclobbered -Wcomment -Wdisabled-optimization -Wempty-body -Wenum-compare -Wextra -Wfloat-equal -Wformat=2 -Wformat-nonliteral -Wformat-security -Wformat -Wformat-y2k -Wframe-larger-than=8192 -Wignored-qualifiers -Winit-self -Winline -Wlarger-than=4096 -Wlogical-op -Wmain -Wmissing-braces -Wmissing-field-initializers -Wmissing-format-attribute -Wmissing-include-dirs -Wno-long-long -Woverlength-strings -Wpacked-bitfield-compat -Wpacked -Wparentheses -Wpointer-arith -Wredundant-decls -Wreturn-type -Wsequence-point -Wshadow -Wsign-compare -Wstack-protector -Wstrict-aliasing=3 -Wstrict-aliasing -Wswitch-default -Wswitch-enum -Wswitch -Wtrigraphs -Wtype-limits -Wundef -Wuninitialized -Wunsafe-loop-optimizations -Wunused-function -Wunused-label -Wunused-parameter -Wunused-value -Wunused-variable -Wunused -Wvolatile-register-var -Wwrite-strings  -Wc++0x-compat -Wctor-dtor-privacy -Weffc++ -Wnon-virtual-dtor -Woverloaded-virtual -Wreorder -Wsign-promo -Wstrict-null-sentinel
LDFLAGS  =

# Detect the system replicant
CFLAGS   += `pkg-config --cflags replicant`
LDFLAGS  += `pkg-config --libs replicant`

all: lock-object.so lock unlock holder

lock-object.so: lock-object.o
	$(CC) $(CFLAGS) $< -shared -o $@

lock-object.o: lock-object.c
	$(CC) $(CFLAGS) $< -c -fPIC -o $@

lock: lock.cc
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $< -o $@

unlock: unlock.cc
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $< -o $@

holder: holder.cc
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $< -o $@
