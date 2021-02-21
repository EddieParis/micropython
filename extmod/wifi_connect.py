import network

print ("Connects to an access point in dhcp mode")

print("- enter ssid:")
ssid = input()

print("- enter password:")
password = input()

sta_if = network.WLAN(network.STA_IF)
if not sta_if.isconnected():
    print('connecting to network...')
    sta_if.active(True)
    sta_if.connect(ssid, password)
    while not sta_if.isconnected():
        pass

print('network config:', sta_if.ifconfig())
