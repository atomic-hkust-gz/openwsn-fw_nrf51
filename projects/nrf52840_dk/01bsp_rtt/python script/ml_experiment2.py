'''
author Manjiang Cao 
e-mail <mcao999@connect.hkust-gz.edu.cn>
this script trying to using ML to predict the distance based on the second RTT wireless experiment
'''

import numpy as np
import matplotlib.pyplot as plt
import os
from scipy.linalg import svd

from sklearn import linear_model
from sklearn.linear_model import LinearRegression
from sklearn.linear_model import Ridge, Lasso

import torch
import torch.nn as nn
import torch.optim as optim

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

    data = np.vstack((time,rssi))

    return data

def construct_train_and_test_data(data_list):
    train_data = []
    test_data = []
    for i in data_list:
        train_data.append(i[:,:1000])
        test_data.append(i[:,1000:])

    return train_data,test_data

def build_trainset_and_testset(train_data,test_data,n,p):
    train_x = []
    train_y = []
    test_x = []
    test_y = []

    train_x_time_mean = []
    train_x_time_var = []
    train_x_rssi_mean = []
    train_x_rssi_var = []
    train_x_distance = []

    test_x_time_mean = []
    test_x_time_var = []
    test_x_rssi_mean = []
    test_x_rssi_var = []

    for i in range(len(train_data)):
        for j in range(n):
            k = np.random.randint(1000 - p)
            train_x.append(train_data[i,:,k:k+p])
            train_x_time_mean.append(np.mean(train_data[i,0,k:k+p]))
            train_x_time_var.append(np.var(train_data[i,0,k:k+p]))
            train_x_rssi_mean.append(np.mean(train_data[i,1,k:k+p]))
            train_x_rssi_var.append(np.var(train_data[i,1,k:k+p]))
            train_x_distance.append(i)

            train_y.append(i)
        
        k = np.random.randint(1000 - p)
        test_x.append(test_data[i,:,k:k+p])
        test_y.append(i)

        test_x_time_mean.append(np.mean(train_data[i,0,k:k+p]))
        test_x_time_var.append(np.var(train_data[i,0,k:k+p]))
        test_x_rssi_mean.append(np.mean(train_data[i,1,k:k+p]))
        test_x_rssi_var.append(np.var(train_data[i,1,k:k+p]))

        packet = [train_x,train_y,test_x,test_y,
                  train_x_time_mean,train_x_time_var,train_x_rssi_mean,train_x_rssi_var,
                  test_x_time_mean,test_x_time_var,test_x_rssi_mean,test_x_rssi_var,train_x_distance]
    
    return packet


def main():
    sample_times = 10
    distance_list = [i for i in range(16)]
    data_list = []
    train_data = []
    test_data = []
    for i in distance_list:
        data = read_data(i)
        data_list.append(data)

    train_data,test_data = construct_train_and_test_data(data_list)
    train_data = np.array(train_data)       #(16,2,1000)
    test_data = np.array(test_data)         #(16,2,1000)

    packet = build_trainset_and_testset(train_data,test_data,sample_times,500)

    train_x = np.array(packet[0])
    train_y = np.array(packet[1])
    test_x = np.array(packet[2])
    test_y = np.array(packet[3])
    train_x_time_mean = np.array(packet[4])
    train_x_time_var = np.array(packet[5])
    train_x_rssi_mean = np.array(packet[6])
    train_x_rssi_var = np.array(packet[7])
    train_x_distance = np.array(packet[12])

    test_x_time_mean = np.array(packet[8])
    test_x_time_var = np.array(packet[9])
    test_x_rssi_mean = np.array(packet[10])
    test_x_rssi_var = np.array(packet[11])

    train_true_time = []
    for i in range(len(train_x_distance)):
        train_true_time.append(float(train_x_distance[i])*2*16000000/299792458 + 20074.659)

    train_true_time = np.array(train_true_time) 
    print(train_true_time)
    train_merged_array = np.concatenate((train_x_time_mean[:, np.newaxis],train_x_time_var[:, np.newaxis],train_x_rssi_mean[:, np.newaxis],train_x_rssi_var[:, np.newaxis]), axis=1)

    X = train_merged_array
    print(X.shape)
    Y = train_y

    lin_model = LinearRegression()
    lin_model.fit(X, Y)

    ridge_model = Ridge()
    ridge_model.fit(X,Y)

    lasso_model = Lasso()
    lasso_model.fit(X,Y)

    test_merged_array = np.concatenate((test_x_time_mean[:, np.newaxis],test_x_time_var[:, np.newaxis],test_x_rssi_mean[:, np.newaxis],test_x_rssi_var[:, np.newaxis]), axis=1)

    test_x = test_merged_array

    '''
    lin_predictions = lin_model.predict(test_x)
    ridge_predictions = ridge_model.predict(test_x)
    lasso_predictions = lasso_model.predict(test_x)


    plt.figure()
    plt.scatter([i for i in range(len(test_x))],lasso_predictions, c = 'b',label = 'lasso_predictions')
    plt.plot([i for i in range(len(test_x))],[i for i in range(len(test_x))],c = 'r',label = 'true value')
    plt.legend()
    plt.show()
    '''
    print(X.shape)
    print(Y.shape)
    X = torch.from_numpy(X).float()
    print(X)
    Y = torch.from_numpy(Y.reshape(16*sample_times,1)).float()

    class RegressionNet(nn.Module):
        def __init__(self, input_size, hidden_size, output_size):
            super(RegressionNet, self).__init__()
            self.fc1 = nn.Linear(input_size, hidden_size)
            self.relu = nn.ReLU()
            self.fc2 = nn.Linear(hidden_size, hidden_size)
            self.relu = nn.ReLU()
            self.fc3 = nn.Linear(hidden_size, hidden_size)
            self.elu = nn.ELU()
            self.fc4 = nn.Linear(hidden_size, hidden_size)
            self.elu = nn.ELU()
            self.fc5 = nn.Linear(hidden_size, output_size)


        def forward(self, x): 
            out = self.fc1(x)
            out = self.relu(out)
            out = self.fc2(out)
            out = self.relu(out)
            out = self.fc3(out)
            out = self.elu(out)
            out = self.fc4(out)
            out = self.elu(out)
            out = self.fc5(out)
            return out
        

    # 定义模型的超参数
    input_size = 4
    hidden_size = 256
    output_size = 1

    # 创建模型实例
    model = RegressionNet(input_size, hidden_size, output_size)

    # 定义损失函数和优化器
    criterion = nn.MSELoss()
    optimizer = optim.Adam(model.parameters(), lr=0.0001)

    # 进行模型训练
    num_epochs = 10000
    for epoch in range(num_epochs):
        # 前向传播
        outputs = model(X)
        loss = criterion(outputs, Y)

        # 反向传播和优化
        optimizer.zero_grad()
        loss.backward()
        optimizer.step()

        # 打印训练过程中的损失
        print(f'Epoch [{epoch+1}/{num_epochs}], Loss: {loss.item():.4f}')

    # 在测试集上进行预测
    test_x = torch.from_numpy(test_x).float()
    with torch.no_grad():
        predictions = model(test_x)

    # 将预测结果转换为numpy数组
    predictions = predictions.numpy()
    print(predictions)

    plt.figure()
    plt.scatter([i for i in range(len(test_x))],predictions, c = 'b',label = 'ML_predictions')
    plt.plot([i for i in range(len(test_x))],[i for i in range(len(test_x))],c = 'r',label = 'true value')
    plt.legend()
    plt.show()


    

main()
