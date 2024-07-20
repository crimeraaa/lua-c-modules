-- Note that we cannot name this file `dyarray.lua`!
local dyarray = require "dyarray"

a = dyarray.new({0, 0, 0, 0, 0, 0, 0, 0, 0, 0})
print("\nCONSTRUCTION")
print("a               ", a)
print("a:push(50)      ", a:push(50))
print("a:insert(8, 80) ", a:insert(8, 80))
print("a:resize(16)    ", a:resize(16))
print("a:length()      ", a:length()) --> 16

--- METHOD FUNCTIONS ------------------------------------------------------- {{{

print("\nMETHOD FUNCTIONS")
print("a:set(1, 13):get(1)", a:set(1, 13):get(1)) --> 13

---@param a dyarray
local function mess_up_get(a)
    print(a:get(100000))
end

print("a:get(100000)      ", pcall(mess_up_get, a)) --> (index out of range)

--- }}} ------------------------------------------------------------------------

--- METAMETHODS ------------------------------------------------------------ {{{

print("\nMETAMETHODS")
a[2] = 26
print("a[2]", a[2]) --> 26

---@param a dyarray
local function mess_up_index(a)
    print(a[100000])
end

print("a[1000000]", pcall(mess_up_index, a)) --> (index out of range)

--- }}} ------------------------------------------------------------------------

--- PUSH AND INSERT -------------------------------------------------------- {{{

print("\nPUSH AND INSERT")
local b = dyarray.new{10, 20, 30, 40}
print("b               ", b)                --> {10, 20, 30, 40}
print("b:push(50)      ", b:push(50))       --> {10, 20, 30, 40, 50}
print("b:insert(-2, 70)", b:insert(-2, 70)) --> {10, 20, 30, 40, 50, 0, 70, 80}
print("b:resize(4)     ", b:resize(4))

--- }}} ------------------------------------------------------------------------

--- PUSH AND POP --- {{{

local c = dyarray.new()

print("\nPUSH AND POP")
print("c            ", c);
print("c:push(10)   ", c:push(10))
local popped = c:pop()
print("c:pop()      ", popped)
print("c            ", c)

--- }}}
