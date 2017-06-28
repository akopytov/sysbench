#!/usr/bin/env sysbench

-- you can run this script like this:
-- $ ./prime-test.lua --cpu-max-prime=20000 --threads=8 --histogram --report-interval=1 run

sysbench.cmdline.options = {
    -- the default values for built-in options are currently ignored, see 
    -- https://github.com/akopytov/sysbench/issues/151
    ["cpu-max-prime"] = {"CPU maximum prime", 10000},
    ["threads"] = {"Number of threads", 1},
    ["histogram"] = {"Show histogram", "off"},
    ["report-interval"] = {"Report interval", 1}
}

function event()
    local n = 0
    for c = 3, sysbench.opt.cpu_max_prime do
        local t = math.sqrt(c)
        local isprime = true
        for l = 2, t do
            if c % l == 0 then
                isprime = false
                break
            end
        end
        if isprime then
            n = n + 1
        end
    end
end

function sysbench.hooks.report_cumulative(stat)
    local seconds = stat.time_interval
    print(string.format([[
{
    "errors": %4.0f,
    "events": %4.0f,
    "latency_avg": %4.10f,
    "latency_max": %4.10f,
    "latency_min": %4.10f,
    "latency_pct": %4.10f,
    "latency_sum": %4.10f,
    "other": %4.0f,
    "reads": %4.0f,
    "reconnects": %4.0f,
    "threads_running": %4.0f,
    "time_interval": %4.10f,
    "time_total": %4.10f,
    "writes": %4.0f
}
]], 
    stat.errors, 
    stat.events, 
    stat.latency_avg, 
    stat.latency_max, 
    stat.latency_min, 
    stat.latency_pct, 
    stat.latency_sum, 
    stat.other, 
    stat.reads, 
    stat.reconnects, 
    stat.threads_running, 
    stat.time_interval, 
    stat.time_total, 
    stat.writes))
end
