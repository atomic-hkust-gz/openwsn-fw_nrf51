import serial
import struct
from config import *
from calc_angle import *
import matplotlib.pyplot as plt

latest_angle = [None, None]

def start_read():

    sample = {
        'magnitude' : 0,
        'phase'     : 0,
    }
    samples = []

    input_data = []
    count = num_new_read_mark
    new_sample_read = False
    
    s = serial.Serial(nrf5340_port, baudrate=115200)

    data = {'samples':[]}

    while True:

        c = s.read(1)
        input_data.append(c)
        
        if ord(c) == 255:
            count -= 1
        else:
            count = num_new_read_mark
        
        if count == 0:
            new_sample_read = True
            count = num_new_read_mark
        
        if new_sample_read:
            new_sample_read = False
            if len(input_data) == NUM_SAMPLES*4+10:
                for i in range(NUM_SAMPLES):
                    chunk = b''.join(input_data[4*i: 4*(i+1)])
                    magnitude, phase = struct.unpack('>Hh', bytes(chunk))
                    sample['magnitude'] = magnitude
                    sample['phase']     = phase
                    data['samples'].append(sample.copy())
                data['rssi']        = struct.unpack('>b', input_data[NUM_SAMPLES*4])
                data['setting']     = ((ord(input_data[NUM_SAMPLES*4+1])) << 10 ) | ((ord(input_data[NUM_SAMPLES*4+2])) << 5) | (ord(input_data[NUM_SAMPLES*4+3]))
                data['array']       = ord(input_data[NUM_SAMPLES*4+4])
                data['angle']       = ord(input_data[NUM_SAMPLES*4+5])
                
                print('rssi:', data['rssi'])
                print('setting:', data['setting'])
                print('array:', data['array'])
                print('angle:', data['angle'])
                
                phase_data  = []
                for sample in data['samples']:
                    phase_data.append(sample['phase'])
                
                fig = plt.figure()
                plt.plot([i*0.125 for i in range(160)],phase_data, marker='*')
                '''
                plt.plot([64*0.125]*2,[-201,201],c='r')
                plt.plot([80*0.125]*2,[-201,201],c='r')
                plt.plot([96*0.125]*2,[-201,201],c='r')
                plt.plot([112*0.125]*2,[-201,201],c='r')
                plt.plot([128*0.125]*2,[-201,201],c='r')
                plt.plot([144*0.125]*2,[-201,201],c='r')
                '''
                i = 8
                plt.plot([8*i*0.125]*2, [-201,201],c = 'b')
                plt.plot([8*(i+1)*0.125]*2, [-201,201],c = 'b')
                plt.plot([8*(i+4)*0.125]*2, [-201,201],c = 'b')
                plt.plot([8*(i+5)*0.125]*2, [-201,201],c = 'b')
                plt.plot([8*(i+8)*0.125]*2, [-201,201],c = 'b')
                plt.plot([8*(i+9)*0.125]*2, [-201,201],c = 'b')

                i = 8+2
                plt.plot([8*i*0.125]*2, [-201,201],c = 'g')
                plt.plot([8*(i+1)*0.125]*2, [-201,201],c = 'g')
                plt.plot([8*(i+4)*0.125]*2, [-201,201],c = 'g')
                plt.plot([8*(i+5)*0.125]*2, [-201,201],c = 'g')
                plt.plot([8*(i+8)*0.125]*2, [-201,201],c = 'g')
                plt.plot([8*(i+9)*0.125]*2, [-201,201],c = 'g')


                plt.plot([160*0.125]*2,[-201,201],c='r')
                plt.xlabel('us')
                plt.ylabel('phase')
                plt.title('{}'.format(data['array']))
                plt.savefig('phase_array{}.png'.format(data['array']))
                
                _, angle = aoa_angle_calculation(phase_data, 0, data['array'])
                
                if data['angle'] != 254:
                    latest_angle[int(data['array'])-1] = data['angle']
                
                with open(sample_file, 'a') as f:
                    f.write(str(data)+'\n')
                    data['samples'] = []
            else:
                print (len(input_data))
                
            input_data = []