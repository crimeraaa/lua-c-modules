CC 		 := cl
CC_FLAGS := -nologo -EHsc -std:c++17 -W3 -WX -Fe"bin/" -Fo"obj/"

.PHONY: all
all: debug
	
# /Od		disable optimizations (default).
# /Zi		enable debugging information.
# /D<macro>	define <macro> for this compilation unit.
# 			NOTE: #define _DEBUG in MSVC requires linking to a debug lib.
# /MDd		linked with MSVCRTD.LIB debug lib. Also defines _DEBUG.
.PHONY: debug
debug: CC_FLAGS += -Od -DBIGINT_DEBUG
debug: build
	
# /O1		maximum optimizations (favor space)
# /O2		maximum optimizations (favor speed)
# /Os		favor code space
# /Ot 		favor code speed
.PHONY: release
release: CC_FLAGS += -O1
release: build
	
.PHONY: build
build: bin/big_int.exe

bin obj:
	$(MKDIR) $@

bin/big_int.exe: src/big_int.cpp src/common.hpp | bin obj
	$(CC) $(CC_FLAGS) $<

.PHONY: clean
clean:
	$(RM) bin/big_int.exe obj/big_int.obj
