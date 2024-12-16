import os

def list_all_directories(path):
    for root, dirs, files in os.walk(path):
        if 'Device_Startup' in dirs:
            for dir_name in dirs:
                if dir_name != 'Device_Startup':
                    print(os.path.join(root, dir_name) + ' \\')


list_all_directories('../../Microchip_Examples')