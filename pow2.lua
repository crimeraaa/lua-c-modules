local prefixes = {"0x1", "0x2", "0x4", "0x8"}
local maxzeros = tonumber(arg[1]) or 4
local hexes    = {n = 0}

-- For each some amount of zeroes...
for nzeroes = 0, maxzeros, 1 do
    -- For each "0x" <n> prefix...
    for _, prefix in ipairs(prefixes) do
        local t = {n = 1, prefix}
        -- Populate an individual prefix with the current amount of zeroes.
        for _ = 1, nzeroes, 1 do
            t.n = t.n + 1
            t[t.n] = '0'
        end
        hexes.n = hexes.n + 1
        hexes[hexes.n] = table.concat(t)
    end
end

for i, v in ipairs(hexes) do
    print(i, v, "=>", tonumber(v))
end
