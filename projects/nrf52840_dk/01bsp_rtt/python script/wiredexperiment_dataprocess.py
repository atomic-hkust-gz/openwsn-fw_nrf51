import numpy as np
import matplotlib.pyplot as plt
import os


def read_data(distance):
    f = open ('C:/Users/11422/Desktop/work_on_nrf52840/openwsn-fw/projects/nrf52840_dk/01bsp_rtt/wiredexperiment/distance{}.txt'.format(distance), 'r')
    time_list = []
    rssi_list = []
    data = f.readlines()
    #print(data)
    for i in range(len(data)):
        time_list.append(int(data[i].split(' ')[0]))
        rssi_list.append(int(data[i].split(' ')[1]))

    time = np.array(time_list)
    rssi = np.array(rssi_list)
    return time,rssi

distance_list = [1,2,3,4,5,6,7,8,9,10,11]

data_time_list = []
data_rssi_list = []
data_time_mean_list = []
data_rssi_mean_list = []
data_time_var_list = []
data_rssi_var_list = []

for i in distance_list:
    time,rssi = read_data(i)
    data_time_list.append(time)
    data_rssi_list.append(rssi)

for i in data_time_list:
    #print(i)
    data_time_mean_list.append(i.mean())
    data_time_var_list.append(i.var())

for i in data_rssi_list:
    data_rssi_mean_list.append(i.mean())
    data_rssi_var_list.append(i.var())

based_mean_time = data_time_mean_list[0]
for i in range(len(data_time_mean_list)):
    data_time_mean_list[i] = ((data_time_mean_list[i] - 20074.659)/2)/16000000*299792458




fig = plt.figure()
ax1 = fig.add_subplot(211)
ax1.plot(distance_list,data_time_mean_list,c = 'r', label = 'mean time')
ax1.set_xlabel('distance')
ax1.set_ylabel('RTT distance')
plt.legend()
#ax2 = fig.add_subplot(222)
#ax2.plot(distance_list,data_time_var_list,c = 'b', label = 'varance of time')
#ax2.set_xlabel('distance')
#plt.legend()
ax3 = fig.add_subplot(212)
ax3.plot(distance_list,data_rssi_mean_list,c = 'r', label = 'mean rssi')
ax3.set_xlabel('distance')
ax3.set_ylabel('rssi')
plt.legend()
#ax4 = fig.add_subplot(224)
#ax4.plot(distance_list,data_rssi_var_list,c = 'b', label = 'varance of rssi')
#ax4.set_xlabel('distance')
plt.legend()
plt.show()


