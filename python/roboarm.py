import serial
import time

DIR_1 = '1'
DIR_2 = '2'

PART_BASE='1'
PART_ELBOW='2'
PART_WRIST='3'
PART_CLAW='4'




class Robo:
    ser=None
    partlookup = {'1':"BASE",'2':"ELBOW",'3':"WRIST",'4':"CLAW"};
    def __init__(self):
        pass
         

    def Initialise(self,com):
        Robo.ser = serial.Serial(com,115200) 

        if(Robo.ser.isOpen() == False):
            Robo.ser.open()

        print("Initialising...")
        time.sleep(5)
        Robo.ser.write(bytes('m','utf-8'))#'m' switches from joystick mode to serial mode
        Robo.ser.flushOutput()
        time.sleep(.1)
        print("Starting...")

    def Send(self,part,direction,seconds):
        if(part!=PART_BASE and part!=PART_ELBOW and part!=PART_WRIST and part!=PART_CLAW ):
            print('Invalid part ', part)
            return
        if(direction !=DIR_1 and direction !=DIR_2):
            print('invalid direction ',direction)
            return

        print(Robo.partlookup[part] + " direction ", direction + " seconds ", seconds)   
    
        Robo.ser.write(bytes(part,'utf-8'))
        Robo.ser.flushOutput()
        time.sleep(.1)
        Robo.ser.write(bytes(direction,'utf-8'))
        Robo.ser.flushOutput()
        time.sleep(.1)
    
        time.sleep(seconds)
        Robo.ser.write(bytes('s','utf-8'))
        Robo.ser.flushOutput()
        time.sleep(.1)


    def Close(self):
        Robo.ser.close()
        print("Closed")
