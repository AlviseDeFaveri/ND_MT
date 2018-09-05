b main

b 715
b 748
b 749

commands 2
  p malloc_stats()
end

commands 3
  p malloc_stats()
end

commands 4
  p malloc_stats()
end

# enter and finished processInput()
#b mt.c:28
#b mt.c:29

# Giro mt
#b mtevolve.h:97
#commands 4
#  p list()
#  c
#end

# Singola evoluzione
#b mtevolve.h:119
#commands 5
#  p mtape(6)
# end

# Not accept
#b mtevolve.h:136
#commands 6
#  p TRACE("------NOT ACCEPT------")
#  p nMt
#  p list()
#  p tapeDump(globTape, 1)
#end

run
c