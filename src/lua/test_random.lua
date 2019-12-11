function thread_init()
   h = sysbench.histogram.new(1000, 1, 10)
end

function event()
   h:update(sysbench.rand.default(1, 10))
end

function thread_done()
   h:print()
end

-- command to run it
--$ sysbench /tmp/random.lua --time=1 --rand-type=zipfian --verbosity=0 run
--$ sysbench /tmp/random.lua --time=1 --rand-type=pareto --verbosity=0 run
--$ sysbench /tmp/random.lua --time=1 --rand-type=special --verbosity=0 run
--$ sysbench /tmp/random.lua --time=1 --rand-type=zipfian --rand-zipfian-exp=0 --verbosity=0 run