# Location of `lua5.1.dll`, `lua5.1.lib`, `lua.h` and friends.
# These were prebuilts downloaded from SourceForge.
LUA_DIR  := C:/Lua51
DIR_SRC	 := src
DIR_OBJ  := obj
DIR_LIB  := .
DIR_ALL	 := $(DIR_OBJ) $(DIR_LIB)

# NAMES 	 := $(patsubst  $(DIR_SRC)/%.c, %, $(wildcard $(DIR_SRC)/*.c))
NAMES	 := dyarray
OUT_DLLS := $(addprefix $(DIR_LIB)/, $(NAMES:=.dll))
OUT_LIBS := $(addprefix $(DIR_LIB)/, $(NAMES:=.lib))
OUT_OBJS := $(addprefix $(DIR_OBJ)/, $(NAMES:=.obj))
OUT_ALL  := $(OUT_DLLS) $(NAMES:=.exp) $(OUT_LIBS) $(OUT_OBJS)

# Note that for some reason forward slash argument syntax doesn't work, at least
# with this installation of GNU Make on Windows (via winget).
#
# /nologo	Silence the `Microsoft copyright` blah blah blah business,
# /EHs		Exception Handling: C++
# /EHc  	Exception Handling: extern "C" defaults to nothrow
# /std:c11	Adhere to the 2011 C Standard.
# /W3		Enable many warnings.
# /WX		Treat all warning as errors.
# /O1		Maximum optimizations, favoring space.
# /O2		Maximum optimizations, favoring speed.
# /Os		Optimize but favoring code space.
# /Od		Disable all optimizations.
# -I"..."	Add an include directory to look at.
CC 	  	 := cl
CC_FLAGS := -nologo -EHsc -std:c11 -W3 -Od -I"$(LUA_DIR)"

.PHONY: all
all: $(OUT_DLLS)
	
# Create directories if they don't exist already.
$(DIR_ALL):
	$(MKDIR) $@
	
# -Fo:"path/"	Set "path/" to be the output directory. Note that if we don't
#				Have the trailing slash, this becomes the output filename.
# /LD			Create a DLL and its associated files.
# /link ...		Pass the remaining arguments to LINK.EXE.
$(DIR_LIB)/%.dll: $(DIR_SRC)/%.c | $(DIR_OBJ)
	$(CC) $(CC_FLAGS) -Fo"$(DIR_OBJ)/" -LD $< -link "$(LUA_DIR)/lua5.1.lib"
	
.PHONY: clean
clean:
	$(RM) $(OUT_ALL)
