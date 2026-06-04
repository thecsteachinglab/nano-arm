from roboarm import *

COM = "COM3"

bot = Robo()
bot.Initialise(COM)

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


bot.Close()





 
 


