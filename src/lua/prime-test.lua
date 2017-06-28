#!/usr/bin/env sysbench

-- you can run this script like this:
-- $ ./prime-test.lua --cpu-max-prime=20000 --threads=8 --histogram --report-interval=1 run

sysbench.cmdline.options = {
    -- the default values for built-in options are currently ignored, see 
    -- https://github.com/akopytov/sysbench/issues/151
    ["cpu-max-prime"] = {"CPU maximum prime", 10000},
    ["threads"] = {"Number of threads", 2},
    ["histogram"] = {"Show histogram", "on"},
    ["report-interval"] = {"Report interval", 1}
}

function event()
    n = 0
    for c = 3, sysbench.opt.cpu_max_prime do
        t = math.sqrt(c)
        l = 2
        for l = 2, t do
            if c % l == 0 then
                break
            end
        end
        if l > t then
            n = n + 1
        end
    end
end

function sysbench.hooks.report_intermediate(stat)
    local seconds = stat.time_interval
    print(string.format("%.0f;%u;%4.2f",
        stat.time_total, 
        stat.threads_running,
        stat.events / seconds))
end