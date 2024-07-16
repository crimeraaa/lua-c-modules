# Location of `lua5.1.dll`, `lua5.1.lib`, `lua.h` and friends.
# These were prebuilts downloaded from SourceForge.
LUA_DIR  := C:/Lua51
LUA_LIBS := array

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
all: $(LUA_LIBS:=.dll)
	
# /LD		Create a DLL and its associated files.
# /link ...	Pass the remaining arguments to LINK.EXE.
%.dll: %.c
	$(CC) $(CC_FLAGS) -LD $< -link "$(LUA_DIR)/lua5.1.lib"
	
.PHONY: clean
clean:
	$(RM) $(LUA_LIBS:=.dll) $(LUA_LIBS:=.exp) $(LUA_LIBS:=.lib) $(LUA_LIBS:=.obj)
