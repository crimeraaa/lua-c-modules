CC 		 := cl
CC_FLAGS := -nologo -EHsc -std:c++17 -W3 -WX -Fe"bin/" -Fo"obj/"

.PHONY: all
all: bin/big_int.exe

bin obj:
	$(MKDIR) $@

bin/%.exe: src/%.cpp | bin obj
	$(CC) $(CC_FLAGS) $<
	
.PHONY: clean
clean:
	$(RM) bin/big_int.exe obj/big_int.obj
