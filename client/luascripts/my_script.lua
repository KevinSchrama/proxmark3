local getopt = require('getopt')
local ansicolors  = require('ansicolors')

local function main(args)
    local uid = 1

    print('Start simulation of LF cards\n')

    core.console('lf hid sim -r 2006ec0c86')
    os.execute('sleep 2')
    core.console('lf simstop')

    core.console('lf em 410x sim --id 0F0368568B')
    os.execute('sleep 1')
    core.console('lf simstop')

    core.console('lf awid sim --fmt 26 --fc 234 --cn 1337')
    os.execute('sleep 1')
    core.console('lf simstop')

    core.console('lf io sim --vn 1 --fc 101 --cn 1338')
    os.execute('sleep 1')
    core.console('lf simstop')

    core.console('lf noralsy sim --cn 1339')
    os.execute('sleep 1')
    core.console('lf simstop')

    core.console('lf paradox sim -r 0f55555695596a6a9999a59a')
    os.execute('sleep 1')
    core.console('lf simstop')

    print('Start simulation of HF cards\n')

    core.console('hf iclass sim -t 0 --csn 4B6576696EB93314')
    os.execute('sleep 1')

    for type = 1, 9, 1 do

        if type ~= 3 and type ~= 4 and type ~= 5 then
        -- send command to proxmark
        core.console('hf 14a sim -t '..type..' -u 4B6576696E000'..uid)
        os.execute('sleep 1')
        uid = uid + 1
        end
    end

end

main(args)