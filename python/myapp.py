from roboarm import *

#important: this number must match your arduino's COM port
COM = "COM3"

#initialise
bot = Robo()
bot.Initialise(COM)

#send[part, direction, number of seconds]
bot.Send(PART_BASE,DIR_1,2)
bot.Send(PART_BASE,DIR_2,2)

bot.Send(PART_ELBOW,DIR_1,2)
bot.Send(PART_ELBOW,DIR_2,2)

bot.Send(PART_WRIST,DIR_2,2)
bot.Send(PART_WRIST,DIR_1,2)

bot.Send(PART_CLAW,DIR_1,2)
bot.Send(PART_CLAW,DIR_2,2)

bot.Send(PART_BASE,DIR_1,2)
bot.Send(PART_BASE,DIR_2,2)

#close the connection
bot.Close()





 
 


