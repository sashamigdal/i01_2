local csv = {}

-- Load a csv file into a table
-- First column is the key
function csv.read_file(filename)
  local t = {}
  local file = assert(io.open(filename, 'r'))
  local line = file:read()
  local header = csv.read(line)
  for line in file:lines() do
    c = csv.read(line)
    local s = {}
    for k, v in ipairs(c) do
      s[header[k]] = v
    end
    t[c[1]] = s
  end
  file:close()

  return t
end

-- Read a line from a csv file and return in a table
-- http://www.lua.org/pil/20.4.html
function csv.read(s)
  s = s .. ','        -- ending comma
  local t = {}        -- table to collect fields
  local fieldstart = 1
  repeat
    -- next field is quoted? (start with `"'?)
    if string.find(s, '^"', fieldstart) then
      local a, c
      local i  = fieldstart
      repeat
        -- find closing quote
        a, i, c = string.find(s, '"("?)', i+1)
      until c ~= '"'    -- quote not followed by quote?
      if not i then error('unmatched "') end
      local f = string.sub(s, fieldstart+1, i-1)
      table.insert(t, (string.gsub(f, '""', '"')))
      fieldstart = string.find(s, ',', i) + 1
    else                -- unquoted; find next comma
      local nexti = string.find(s, ',', fieldstart)
      local val = string.sub(s, fieldstart, nexti-1)
      local numval = tonumber(val)
      if numval == nil then -- tonumber returns nil if val is not a number
        table.insert(t, numval)
      else
        table.insert(t, val)
      end
      fieldstart = nexti + 1
    end
  until fieldstart > string.len(s)
  return t
end

return csv
