# Location of `lua5.1.dll`, `lua5.1.lib`, `lua.h` and friends.
# These were prebuilts downloaded from SourceForge.
LUA_DIR  := C:/Lua51
DIR_SRC	 := src
DIR_OBJ  := obj
DIR_BIN	 := .
DIR_ALL	 := $(DIR_OBJ) $(DIR_BIN)

# NAMES 	 := $(patsubst  $(DIR_SRC)/%.c, %, $(wildcard $(DIR_SRC)/*.c))
NAMES	 := dyarray
OUT_OBJS := $(addprefix $(DIR_OBJ)/, $(NAMES:=.obj))
OUT_DLLS := $(addprefix $(DIR_BIN)/, $(NAMES:=.dll))
OUT_LIBS := $(addprefix $(DIR_BIN)/, $(NAMES:=.lib))
OUT_EXPS := $(addprefix $(DIR_BIN)/, $(NAMES:=.exp))
OUT_ALL  := $(OUT_DLLS) $(OUT_EXPS) $(OUT_LIBS) $(OUT_OBJS)

# Note that for some reason forward slash argument syntax doesn't work, at least
# with this installation of GNU Make on Windows (via winget).
#
# /nologo		Silence the `Microsoft copyright` blah blah blah business,
# /EHs			Exception Handling: C++
# /EHc  		Exception Handling: extern "C" defaults to nothrow
# /std:c11		Adhere to the 2011 C Standard.
# /W3			Enable many warnings.
# /WX			Treat all warning as errors.
# /O1			Maximum optimizations, favoring space.
# /O2			Maximum optimizations, favoring speed.
# /Os			Optimize but favoring code space.
# /Od			Disable all optimizations.
#
# NOTE: For the following flags (except for /I), if we don't have the trailing
# slash the argument becomes the output filename.
# /I"path/"		Add the directory "path/" to our includes search list.
# /Fe:"path/	Set "path/" to be the output directory for .{exe,dll}.
# /Fo:"path/"	Set "path/" to be the output directory.
CC 	  	 := cl
CC_FLAGS := -nologo -EHsc -std:c11 -W3 -I"$(LUA_DIR)" -Fe"$(DIR_BIN)/" -Fo"$(DIR_OBJ)/"

.PHONY: all
all: debug

.PHONY: debug
debug: CC_FLAGS += -Od -D_DEBUG
debug: build

.PHONY: release
release: CC_FLAGS += -Os
release: build

.PHONY: build
build: $(OUT_DLLS)

# Create directories if they don't exist already.
$(DIR_ALL):
	$(MKDIR) $@

# /LD			Create a DLL and its associated files.
# /link ...		Pass the remaining arguments to LINK.EXE.
$(DIR_BIN)/%.dll: $(DIR_SRC)/%.c $(DIR_SRC)/common.h | $(DIR_BIN) $(DIR_OBJ)
	$(CC) $(CC_FLAGS) -LD $< -link "$(LUA_DIR)/lua5.1.lib"

.PHONY: clean
clean:
	$(RM) $(OUT_ALL)
