require "dyarray"

a = dyarray.new{10, 20, 30, 40}
print("dyarray.new{10, 20, 30, 40} =", a)
print("a:push(50) =", a:push(50))
print("a:insert(8, 80) =", a:insert(8, 80))
print("a:resize(4) =", a:resize(4))
