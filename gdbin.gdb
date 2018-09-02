b main

# enter and finished processInput()
b mt.c:28
b mt.c:29

# Giro mt
b mtevolve.h:97
commands 4
  p list()
  c
end

# Singola evoluzione
b mtevolve.h:119
commands 5
  p mtape(6)
 end

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