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
---@param len integer
function dyarray:resize(len)
    self.m_capacity = len
    -- If we extended, zero out the extended region.
    for i = self.m_length + 1, len, 1 do
        self.m_values[i] = 0
    end
    return self
end

---@param i integer
---@param v number
function dyarray:insert(i, v)
    -- Need to extend the buffer?
    if i > self.m_length then
        self:resize(i)
    end
    self.m_values[i] = v
    return self
end

---@param v number
function dyarray:push(v)
    return self:insert(self.m_length + 1, v)
end

function dyarray:length()
    return self.m_length
end
