#localRelayKey = 0xAA
#localCameraKey = 0xAA
#localThermostatKey = 0xAA
#localLockKey = 0xAA
#localMotionKey = 0xAA

import time
import pyrebase
import spidev
import random

#SPI init

# We only have SPI bus 0 available to us on the Pi
bus = 0

#Device is the chip select pin. Set to 0 or 1, depending on the connections
device = 0

# Enable SPI
spi = spidev.SpiDev()

# Open a connection to a specific bus and device (chip select pin)
spi.open(bus, device)

# Set SPI speed and mode
spi.max_speed_hz = 100000
spi.mode = 0

print("- - SPI initialized successfully - -")
#LoraInit()
#setRxMode()
#print("- - LoRa initialized successfully - -")
localRelayKey = 0xAA
localCameraKey = 0xAA
localThermostatKey = 0xAA
localLockKey = 0xAA
localMotionKey = 0xAA


#Lora Commands
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
def LoraInit():
    #config LoRa
    #print("writing 0x80 (sleep, highFrq,LoRa) to reg config (0x01)")
    msg = [0x80 | 0x01, 0x80]
    result = spi.xfer2(msg)

    #high power tx mode
   # print("writing 0xFF to reg high power TX (0x09)")
    msg = [0x80 | 0x09,0xFF]
    result = spi.xfer2(msg)

    #verify Output Pin configuration
    #print("writing 0x00 to output PIN connection (0x40)")
    msg = [0x80 | 0x40, 0x00]
    result = spi.xfer2(msg)
   #print("Output Pin Config -> msg[0]: ",hex(msg[0]),"msg[1]: ",hex(msg[1]))
   # print("Lora Initilized Succesfully")
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
def CheckMessage():
    msg = [0x00 | 0x12, 0x00]
    result = spi.xfer2(msg)
    if(result[1] & 0x40 == 0x40): #0x40 = Rx recived!
        #verify legitness
        msg = [0x00 | 0x13, 0x00]
        result = spi.xfer2(msg)
        numBytesReceived = result[1]
        if numBytesReceived > 10:
            #clear flag
            msg = [0x80 | 0x12, 0xFF]
            result = spi.xfer2(msg)
            return False
        return True
    elif(result[1] & 0x80 == 0x80 or result[1] & 0x20 == 0x20):
        #0x80 = RX timeout
        #0x50 = CRC error
        
        #clear flag
        msg = [0x80 | 0x12, 0xFF]
        result = spi.xfer2(msg)
        return False
    else:
        return False
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
def readMessage():
    #assume 'CheckMessage()' returned true
    
    #clear flag
    msg = [0x80 | 0x12, 0xFF]
    result = spi.xfer2(msg)

    #extract data - - - 
    msg = [0x00 | 0x13, 0x00]
    result = spi.xfer2(msg)
    numBytesReceived = result[1]
    msg = [0x00 | 0x10, 0x00]
    result = spi.xfer2(msg)
    storageLocation = result[1]

    #set fifo to storage location
    msg = [0x80 | 0x0D, storageLocation]
    result = spi.xfer2(msg)

    storageArray = []
    #extract data
    for x in range(numBytesReceived):
        msg = [0x00 | 0x00, 0x00]
        result = spi.xfer2(msg)
        storageArray.append(result[1])

    #reset FIFO ptr
    msg = [0x80 | 0x0D, 0x00]
    result = spi.xfer2(msg)

    return storageArray
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
def setRxMode():
    #print("writing SLEEP mode (0x81) to config register (0x01)")
    msg = [0x80 | 0x01,0x80]
    result = spi.xfer2(msg)

    #setup Rx fifo
    msg = [0x80 | 0x0D,0x00]
    result = spi.xfer2(msg)

    #set into Rx Continous Mode
    msg = [0x80 | 0x01,0x85]
    result = spi.xfer2(msg)
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
def LoraSendMessage( messageList, messageSize):
    #print("writing STBY mode (0x81) to config register (0x01)")
    msg = [0x80 | 0x01, 0x81]
    result = spi.xfer2(msg)

    #print("writing Tx_fifoPtr (0x80) to FifoPtr_address (0x0D)")
    msg = [0x80 | 0x0D,0x80]
    result = spi.xfer2(msg)

    #fillFifo
    for x in messageList:
        msg = [0x80 | 0x00,x]
        result = spi.xfer2(msg)
    #print("set payload length (in our case 0x03) to register (0x22)")
    msg = [0x80 | 0x22,messageSize]
    result = spi.xfer2(msg)

    #print("Put tranciver into TX mode")
    msg = [0x80 | 0x01, 0x83]
    result = spi.xfer2(msg)

    #wait until Tx is done
    msg = [0x00 | 0x12, 0x00]
    result = spi.xfer2(msg) 
    while(result[1] & 0x08 != 0x08):
        msg = [0x00 | 0x12, 0x00]
        result = spi.xfer2(msg)

    #clear Tx flag
    msg = [0x80 | 0x12, 0x08]
    result = spi.xfer2(msg) 

    #print
    print("LoRa       | Sent Message: {",end =" ")
    for x in messageList:
        print(hex(x),end =" ")
    print("}")
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
def updateKey():
    newKey = random.randint(1,255)
#    msg1 = [0x02, 0x04, newKey,localKey_perModule]
#    LoraSendMessage(msg1, 4)
    return newKey






LoraInit()
setRxMode()
print("- - LoRa initialized successfully - -")




#fireBase user authentication Config
config = {
  "apiKey": "AIzaSyDDEU0H3zGgVTUMgNayaV2i5bTopef2ano",
  "authDomain": "vesta-d10e5.firebaseapp.com",
  "databaseURL": "https://vesta-d10e5.firebaseio.com",
  "projectId": "vesta-d10e5",
  "storageBucket": "vesta-d10e5.appspot.com",
  "messagingSenderId": "647724162108",
  #"serviceAccount": "vesta.json"
}

#init firebase
firebase = pyrebase.initialize_app(config)
#authenticate firebase
auth = firebase.auth()
user = auth.sign_in_with_email_and_password("nlenze@umich.edu", "vesta123")
#define database
db = firebase.database()


#App Listeners


#camera streams
def camera_delete_Handler(message):
    print("Camera     | Delete Recordings (Received from App)")

    msg = [0x02, 0x03, localCameraKey]
    LoraSendMessage(msg, 3)
    setRxMode()
    

    #key Update
    time.sleep(5)
    global localCameraKey
    tempKey = updateKey()
    msg1 = [0x02, 0x04, tempKey,localCameraKey]
    print("Camera     | Key Updated:",hex(tempKey))
    LoraSendMessage(msg1, 4)
    localCameraKey = tempKey
    setRxMode()
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
def camera_livestream_Handler(message):
    print("Camera     | Livestream",message["data"],"(Received from App)")

    if message["data"] == "Off":
        msg = [0x02, 0x02, 0x00, localCameraKey]
    else: #ie, livestream is on
        msg = [0x02, 0x02, 0x01, localCameraKey] 
    #msg = [0x02, 0x02, 0xab]
    LoraSendMessage(msg, 4)
    setRxMode()

    
    #key Update
    time.sleep(5)
    global localCameraKey
    tempKey = updateKey()
    msg1 = [0x02, 0x04, tempKey,localCameraKey]
    print("Camera     | Key Updated:",hex(tempKey))
    LoraSendMessage(msg1, 4)
    localCameraKey = tempKey
    setRxMode()
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
def camera_recording_Handler(message):
    print("Camera     | Recording",message["data"],"(Received from App)")

    if message["data"] == "Off":
        msg = [0x02, 0x01, 0x00, localCameraKey]
    else:
        msg = [0x02, 0x01, 0x01, localCameraKey]
    #msg = [0x02, 0x01, 0xab]
    LoraSendMessage(msg, 4)
    setRxMode()
    

    #key Update
    time.sleep(5)
    global localCameraKey
    tempKey = updateKey()
    msg1 = [0x02, 0x04, tempKey,localCameraKey]
    print("Camera     | Key Updated:",hex(tempKey))
    LoraSendMessage(msg1, 4)
    localCameraKey = tempKey
    setRxMode()
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

cam_livestream_stream = db.child("Modules").child("Camera").child("Live stream").stream(camera_livestream_Handler, user['idToken'])
cam_recording_stream = db.child("Modules").child("Camera").child("Recording").stream(camera_recording_Handler, user['idToken'])
cam_delete_stream = db.child("Modules").child("Camera").child("DeleteRec").stream(camera_delete_Handler, user['idToken'])


#lock streams
def lock_state_Handler(message):
    print("Lock       | State:",message["data"],"(Received from App)")
    if(message["data"] == "Unlocked"):
        msg = [0x03, 0x01, 0x00 localCameraKey]
        LoraSendMessage(msg, 4)
        setRxMode()
    else: #locked --- LOCKED = 0x01 ---
        msg = [0x03, 0x01, 0x01 localCameraKey]
        LoraSendMessage(msg, 4)
        setRxMode()
    
    

    #key Update
    time.sleep(5)
    global localLockKey
    tempKey = updateKey()
    msg1 = [0x03, 0x04, tempKey,localLockKey]
    print("Lock       | Key Updated:",hex(tempKey))
    LoraSendMessage(msg1, 4)
    localLockKey = tempKey
    setRxMode()

lock_state_stream = db.child("Modules").child("Lock").child("State").stream(lock_state_Handler, user['idToken'])

#motion streams (DON'T NEED IT!)

#Relay streams
def relay_0_Handler(message):
    print("Relay 0    | Mode:",message["data"],"(Received from App)")
    if message["data"] == "Off":
        msg = [0x05, 0x01, 0x00, localRelayKey]
    else:
        msg = [0x05, 0x01, 0x01, localRelayKey]
    #msg = [0x02, 0x01, 0xab]
    LoraSendMessage(msg, 4)
    setRxMode()


    #key Update
    time.sleep(5)
    global localRelayKey
    tempKey = updateKey()
    msg1 = [0x05, 0x04, tempKey,localRelayKey]
    print("Relay 0    | Key Updated:",hex(tempKey))
    LoraSendMessage(msg1, 4)
    localRelayKey = tempKey
    setRxMode()
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
def relay_1_Handler(message):
    print("Relay 1    | Mode:",message["data"],"(Received from App)")
    if message["data"] == "Off":
        msg = [0x05, 0x02, 0x00, localRelayKey]
    else:
        msg = [0x05, 0x02, 0x01, localRelayKey]
    #msg = [0x02, 0x01, 0xab]
    LoraSendMessage(msg, 4)
    setRxMode()

    #key Update
    time.sleep(5)
    global localRelayKey
    tempKey = updateKey()
    msg1 = [0x05, 0x04, tempKey,localRelayKey]
    print("Relay 1    | Key Updated:",hex(tempKey))
    LoraSendMessage(msg1, 4)
    localRelayKey = tempKey
    setRxMode()
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
def relay_2_Handler(message):
    print("Relay 2    | Mode:",message["data"],"(Received from App)")
    if message["data"] == "Off":
        msg = [0x05, 0x03, 0x00, localRelayKey]
    else:
        msg = [0x05, 0x03, 0x01, localRelayKey]
    #msg = [0x02, 0x01, 0xab]
    LoraSendMessage(msg, 4)
    setRxMode()

    #key Update
    time.sleep(5)
    global localRelayKey
    tempKey = updateKey()
    msg1 = [0x05, 0x04, tempKey,localRelayKey]
    print("Relay 2    | Key Updated:",hex(tempKey))
    LoraSendMessage(msg1, 4)
    localRelayKey = tempKey
    setRxMode()
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
def relay_3_Handler(message):
    print("Relay 3    | Mode:",message["data"],"(Received from App)")
    if message["data"] == "Off":
        msg = [0x05, 0x04, 0x00, localRelayKey]
    else:
        msg = [0x05, 0x04, 0x01, localRelayKey]
    #msg = [0x02, 0x01, 0xab]
    LoraSendMessage(msg, 4)
    setRxMode()

    #key Update
    time.sleep(5)
    global localRelayKey
    tempKey = updateKey()
    msg1 = [0x05, 0x04, tempKey,localRelayKey]
    print("Relay 3    | Key Updated:",hex(tempKey))
    LoraSendMessage(msg1, 4)
    localRelayKey = tempKey
    setRxMode()
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
def relay_4_Handler(message):
    print("Relay 4    | Mode:",message["data"],"(Received from App)")
    if message["data"] == "Off":
        msg = [0x05, 0x05, 0x00, localRelayKey]
    else:
        msg = [0x05, 0x05, 0x01, localRelayKey]
    #msg = [0x02, 0x01, 0xab]
    LoraSendMessage(msg, 4)
    setRxMode()

    #key Update
    time.sleep(5)
    global localRelayKey
    tempKey = updateKey()
    msg1 = [0x05, 0x04, tempKey,localRelayKey]
    print("Relay 4    | Key Updated:",hex(tempKey))
    LoraSendMessage(msg1, 4)
    localRelayKey = tempKey
    setRxMode()
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
def relay_5_Handler(message):
    print("Relay 5    | Mode:",message["data"],"(Received from App)")
    if message["data"] == "Off":
        msg = [0x05, 0x06, 0x00, localRelayKey]
    else:
        msg = [0x05, 0x06, 0x01, localRelayKey]
    #msg = [0x02, 0x01, 0xab]
    LoraSendMessage(msg, 4)
    setRxMode()

    #key Update
    time.sleep(5)
    global localRelayKey
    tempKey = updateKey()
    msg1 = [0x05, 0x04, tempKey,localRelayKey]
    print("Relay 5    | Key Updated:",hex(tempKey))
    LoraSendMessage(msg1, 4)
    localRelayKey = tempKey
    setRxMode()
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
def relay_6_Handler(message):
    print("Relay 6    | Mode:",message["data"],"(Received from App)")
    if message["data"] == "Off":
        msg = [0x05, 0x07, 0x00, localRelayKey]
    else:
        msg = [0x05, 0x07, 0x01, localRelayKey]
    #msg = [0x02, 0x01, 0xab]
    LoraSendMessage(msg, 4)
    setRxMode()

    #key Update
    time.sleep(5)
    global localRelayKey
    tempKey = updateKey()
    msg1 = [0x05, 0x04, tempKey,localRelayKey]
    print("Relay 6    | Key Updated:",hex(tempKey))
    LoraSendMessage(msg1, 4)
    localRelayKey = tempKey
    setRxMode()
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
def relay_7_Handler(message):
    print("Relay 7    | Mode:",message["data"],"(Received from App)")
    if message["data"] == "Off":
        msg = [0x05, 0x08, 0x00, localRelayKey]
    else:
        msg = [0x05, 0x08, 0x01, localRelayKey]
    #msg = [0x02, 0x01, 0xab]
    LoraSendMessage(msg, 4)
    setRxMode()

    #key Update
    time.sleep(5)
    global localRelayKey
    tempKey = updateKey()
    msg1 = [0x05, 0x04, tempKey,localRelayKey]
    print("Relay 7    | Key Updated:",hex(tempKey))
    LoraSendMessage(msg1, 4)
    localRelayKey = tempKey
    setRxMode()

print("- - - - - - - - - - - - - - - - - - - - - - - - - - - -\n                      Prepare Streams\n- - - - - - - - - - - - - - - - - - - - - - - - - - - -")

relay_0_stream = db.child("Modules").child("Relay").child("Channel 0").stream(relay_0_Handler, user['idToken'])
relay_1_stream = db.child("Modules").child("Relay").child("Channel 1").stream(relay_1_Handler, user['idToken'])
relay_2_stream = db.child("Modules").child("Relay").child("Channel 2").stream(relay_2_Handler, user['idToken'])
relay_3_stream = db.child("Modules").child("Relay").child("Channel 3").stream(relay_3_Handler, user['idToken'])
relay_4_stream = db.child("Modules").child("Relay").child("Channel 4").stream(relay_4_Handler, user['idToken'])
relay_5_stream = db.child("Modules").child("Relay").child("Channel 5").stream(relay_5_Handler, user['idToken'])
relay_6_stream = db.child("Modules").child("Relay").child("Channel 6").stream(relay_6_Handler, user['idToken'])
relay_7_stream = db.child("Modules").child("Relay").child("Channel 7").stream(relay_7_Handler, user['idToken'])

#thermostat streams
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
def thermostat_set_temp_Handler(message):
    print("Thermostat | Set Temp:",message["data"],"(Received from App)")

    intTemp = int(message["data"])
    if(intTemp < 0 or intTemp > 255):
        print("Thermostat | !!ERROR!! Set Temp:",message["data"],"(Received from App) is too big, setting to 70")
        intTemp = 70
    msg = [0x06, 0x02, intTemp, localThermostatKey]
    LoraSendMessage(msg, 4)
    setRxMode()
    

    #key Update
    time.sleep(5)
    global localThermostatKey
    tempKey = updateKey()
    msg1 = [0x06, 0x04, tempKey,localThermostatKey]
    print("Thermostat | Key Updated:",hex(tempKey))
    LoraSendMessage(msg1, 4)
    localThermostatKey = tempKey
    setRxMode()
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
def thermostat_mode_Handler(message):
    print("Thermostat | Set Mode:",message["data"],"(Received from App)")
    if( message["data"] == "Heat"): #(0x00 = cool, 0x01 == heat)
        msg = [0x06, 0x03, 0x01, localThermostatKey]
        LoraSendMessage(msg, 4)
        setRxMode()
    else: #mode = cool (0x00 = cool, 0x01 == heat)
        msg = [0x06, 0x03, 0x00, localThermostatKey]
        LoraSendMessage(msg, 4)
        setRxMode()


    #key Update
    time.sleep(5)
    global localThermostatKey
    tempKey = updateKey()
    msg1 = [0x06, 0x04, tempKey,localThermostatKey]
    print("Thermostat | Key Updated:",hex(tempKey))
    LoraSendMessage(msg1, 4)
    localThermostatKey = tempKey
    setRxMode()



thermostat_set_temp_stream = db.child("Modules").child("Thermostat").child("Set Temp").stream(thermostat_set_temp_Handler, user['idToken'])
thermostat_mode_stream = db.child("Modules").child("Thermostat").child("Mode").stream(thermostat_mode_Handler, user['idToken'])
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 




# wait for streams to initilize
time.sleep(10) 
print("- - - - - - - - - - - - - - - - - - - - - - - - - - - -\n                      Streams Ready\n- - - - - - - - - - - - - - - - - - - - - - - - - - - -")
localRelayKey = 0x55
localCameraKey = 0x55
localThermostatKey = 0x55
localLockKey = 0x55
localMotionKey = 0x55

while True:
    #print("hello1")
    if CheckMessage() == True:
        listABC = readMessage()
        print("LoRa       | New Message: {",end =" ")
        for x in listABC:
            print(hex(x),end = " ")
        print("}")


