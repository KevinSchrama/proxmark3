local getopt = require('getopt')
local ansicolors  = require('ansicolors')

local function main(args)
    local uid = 1
    core.console('hf iclass sim -t 0 --csn E012FFF902B93314')
    os.execute('sleep 1')
    for type = 1, 9, 1 do

        print(type)
        if type ~= 3 and type ~= 4 and type ~= 5 then
        -- send command to proxmark
        core.console('hf 14a sim -t '..type..' -u 0000000000000'..uid)
        os.execute('sleep 1')
        uid = uid + 1
        end
    end

end

main(args)