-- This is a mock implementation for the `array` type from `array.c`.
---@class array
---@field values  number[]
---@field length_ integer
array = {}

---@param n integer
---@return array
function array.new(n)
    local inst = {}
    inst.values = {}
    inst.length_ = n -- In C we don't have collision, but in Lua we do.
    return inst
end

---@param i integer
function array:get(i)
    return self.values[i]
end

---@param i integer
---@param v number
function array:set(i, v)
    self.values[i] = v
end


function array:length()
    return self.length_
end
