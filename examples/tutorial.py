import MMCorePy

mmc = MMCorePy.CMMCore()
mmc.loadDevice('tutorial', 'RPiTutorial', 'RPiTutorial')
mmc.initializeAllDevices()

print(mmc.getProperty('tutorial', 'Switch On/Off'))

mmc.setProperty('tutorial', 'Switch On/Off', 'Off')
print(mmc.getProperty('tutorial', 'Switch On/Off'))
