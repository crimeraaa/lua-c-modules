---@meta

-- This is a mock implementation for the `dyarray` type.
-- For more information see `src/dyarray.c`.
---@class dyarray
---@field m_values   number[]
---@field m_length   integer
---@field m_capacity integer
dyarray = {}

---@param t number[]
function dyarray.new(t)
    ---@type dyarray
    local inst      = setmetatable({}, dyarray)
    inst.m_values   = t
    inst.m_length   = #t
    inst.m_capacity = inst.m_length;
    return inst
end

---@param i integer
function dyarray:get(i)
    return self.m_values[i]
end

---@param i integer
---@param v number
function dyarray:set(i, v)
    self.m_values[i] = v
    return self
end

-- Mostly C code, not much Lua-equivalent representation!
---@param sz integer
function dyarray:resize(sz)
    self.m_capacity = sz
    return self
end

---@param i integer
---@param v number
function dyarray:insert(i, v)
    self.m_values[i] = v
    if i > self.m_length then
        self.m_length = i
    end
    return self
end

---@param v number
function dyarray:push(v)
    return self:insert(self.m_length + 1, v)
end

function dyarray:length()
    return self.m_length
end
