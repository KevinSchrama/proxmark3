local getopt = require('getopt')
local ansicolors  = require('ansicolors')

local function isempty(s)
    return s == nil or s == ''
end

local function main(args)

    for o, a in getopt.getopt(args, 't:h') do -- Populate command like arguments
        if o == 't' then typecard = a end
        if o == 'h' then print('t for typecard, 0 is sim stop') end
    end

    if isempty(typecard) then typecard = '1' end

    if typecard == '0' then
        core.console('lf simstop')
    elseif typecard == '1' then
        core.console('lf hid sim -r 3006ec0c86')                    --1006ec0c86
        os.execute('sleep 2')
    elseif typecard == '2' then
        core.console('lf em 410x sim --id 0F0368568B')              --0F0368568B
        os.execute('sleep 1')
    elseif typecard == '3' then
        core.console('lf noralsy sim --cn 1338')                    --fefe
        os.execute('sleep 2')
    elseif typecard == '4' then
        core.console('lf paradox sim -r 0f55555695596a6a9999a59a')  --218277aacb
        os.execute('sleep 1')
    elseif typecard == '5' then
        core.console('hf iclass simfast')                           --4B6576696EB93314
        os.execute('sleep 2')
    elseif typecard == '6' then
        core.console('hf 14a sim -t 1 -u 4B6576696E0006')
        os.execute('sleep 1')
    elseif typecard == '7' then
        core.console('hf 14a sim -t 2 -u 4B6576696E0007')
        os.execute('sleep 1')
    elseif typecard == '8' then
        core.console('hf 14a sim -t 6 -u 4B6576696E0008')
        os.execute('sleep 1')
    elseif typecard == '9' then
        core.console('hf 14a sim -t 7 -u 4B6576696E0009')
        os.execute('sleep 1')
    elseif typecard == '10' then
        core.console('hf 14a sim -t 8 -u 4B6576696E000A')
        os.execute('sleep 1')
    elseif typecard == '11' then
        core.console('hf 14a sim -t 9 -u 4B6576696E000B')
        os.execute('sleep 1')
    end

end

main(args)