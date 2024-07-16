if not package.cpath:find("%.\\%?%.dll") then
    package.cpath = package.cpath .. ";.\\?.dll"
end

-- Note that we cannot name this file `test-array.lua`!
require "array"

a = array.new(10)
print("a:length()", a:length()) --> 10

--- METHOD FUNCTIONS ------------------------------------------------------- {{{

a:set(1, 13)
print("a:get(1)", a:get(1)) --> 13

---@param a array
local function mess_up_get(a)
    print(a:get(100000))
end

print("a:get(100000)", pcall(mess_up_get, a)) --> (index out of range)

--- }}} ------------------------------------------------------------------------

--- METAMETHODS ------------------------------------------------------------ {{{

a[2] = 26
print("a[2]", a[2]) --> 26

---@param a array
local function mess_up_index(a)
    print(a[100000])
end

print("a[1000000]", pcall(mess_up_index, a)) --> (index out of range)

--- }}} ------------------------------------------------------------------------
