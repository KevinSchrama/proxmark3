local getopt = require('getopt')
local ansicolors  = require('ansicolors')

local function main(args)
    local simwaittime = 5
    local waittime = 1
---[[
    print('Start simulation of HF cards\n')

    os.execute('sleep '..waittime)

    core.console('hf iclass sim -t 0 --csn 4B6576696EB93314')
    os.execute('sleep '..simwaittime)
    core.console('hf simstop')

    for type = 1, 9, 1 do

        if type ~= 3 and type ~= 4 and type ~= 5 then
        os.execute('sleep '..waittime)
        -- send command to proxmark
        core.console('hf 14a sim -t '..type..' -u 4B6576696E000'..type)
        os.execute('sleep '..simwaittime)
        core.console('hf simstop')
        end
    end
--]]
---[[
    print('Start simulation of LF cards\n')
    
    os.execute('sleep '..waittime)

    core.console('lf hid sim -r 3006ec0c86')                    --1006ec0c86
    os.execute('sleep '..simwaittime)
    core.console('lf simstop')
    
    os.execute('sleep '..waittime)

    core.console('lf em 410x sim --id 0F0368568B')              --0F0368568B
    os.execute('sleep '..simwaittime)
    core.console('lf simstop')
    
    os.execute('sleep '..waittime)

    core.console('lf awid sim --fmt 26 --fc 234 --cn 1337')     --6d40a73
    os.execute('sleep '..simwaittime)
    core.console('lf simstop')
    
    os.execute('sleep '..waittime)

    core.console('lf io sim --vn 1 --fc 101 --cn 1338')
    os.execute('sleep '..simwaittime)
    core.console('lf simstop')

    os.execute('sleep '..waittime)

    core.console('lf noralsy sim --cn 1338')                    --fefe
    os.execute('sleep '..simwaittime)
    core.console('lf simstop')
    
    os.execute('sleep '..waittime)

    core.console('lf paradox sim -r 0f55555695596a6a9999a59a')  --218277aacb
    os.execute('sleep '..simwaittime)
    core.console('lf simstop')
    
    os.execute('sleep '..waittime)
--]]
end

main(args)