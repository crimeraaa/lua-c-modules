require "dyarray"

local a = dyarray.new{10, 20, 30, 40}
print("dyarray.new{10, 20, 30, 40} =", a)
print("a:insert(5, 50)", a:insert(5, 50))
-- print("a:push(50) =", a:push(50))
