---@diagnostic disable:undefined-global
package.cpath = package.cpath .. ";./?.dll"

-- Open `hello.dll`, call `luaopen_hello()`
-- will load defined functions into global table `hello`
require "hello"

print(_HELLO)
print(hello.say_hello())

