import numpy as np
import matplotlib.pyplot as plt
import os


def read_data(distance):
    f = open ('C:/Users/11422/Desktop/work_on_nrf52840/openwsn-fw/projects/nrf52840_dk/01bsp_rtt/experiment2/distance{}.txt'.format(distance), 'r')
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

distance_list = [i for i in range(16)]

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
    data_time_mean_list[i] = data_time_mean_list[i] - based_mean_time




fig = plt.figure()
ax1 = fig.add_subplot(221)
ax1.plot(distance_list,data_time_mean_list,c = 'r', label = 'mean time')
plt.legend()
ax2 = fig.add_subplot(222)
ax2.plot(distance_list,data_time_var_list,c = 'b', label = 'varance of time')
plt.legend()
ax3 = fig.add_subplot(223)
ax3.plot(distance_list,data_rssi_mean_list,c = 'r', label = 'mean rssi')
plt.legend()
ax3 = fig.add_subplot(224)
ax3.plot(distance_list,data_rssi_var_list,c = 'b', label = 'varance of rssi')
plt.legend()
plt.show()