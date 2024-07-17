if not package.cpath:find("%.\\%?%.dll") then
    package.cpath = package.cpath .. ";.\\?.dll"
end

-- Note that we cannot name this file `test-array.lua`!
require "dyarray"

a = dyarray.new({0, 0, 0, 0, 0, 0, 0, 0, 0, 0})
print(a)
print("a:length()", a:length()) --> 10

--- METHOD FUNCTIONS ------------------------------------------------------- {{{

print("METHOD FUNCTIONS")
print("a:set(1, 13):get(1)", a:set(1, 13):get(1)) --> 13

---@param a dyarray
local function mess_up_get(a)
    print(a:get(100000))
end

print("a:get(100000)", pcall(mess_up_get, a)) --> (index out of range)

--- }}} ------------------------------------------------------------------------

--- METAMETHODS ------------------------------------------------------------ {{{

print("METAMETHODS")
a[2] = 26
print("a[2]", a[2]) --> 26

---@param a dyarray
local function mess_up_index(a)
    print(a[100000])
end

print("a[1000000]", pcall(mess_up_index, a)) --> (index out of range)

--- }}} ------------------------------------------------------------------------

--- PUSH AND INSERT -------------------------------------------------------- {{{

print("PUSH AND INSERT")
local b = dyarray.new{10, 20, 30, 40}
print("b", b)                               --> {10, 20, 30, 40}
print("b:push(50)", b:push(50))             --> {10, 20, 30, 40, 50}
print("b:insert(8, 80)", b:insert(8, 80))   --> {10, 20, 30, 40, 50, 0, 0, 80}
print("b:insert(-2, 70)", b:insert(-2, 70)) --> {10, 20, 30, 40, 50, 0, 70, 80}
print("b:resize(4)", b:resize(4))

--- }}} ------------------------------------------------------------------------
