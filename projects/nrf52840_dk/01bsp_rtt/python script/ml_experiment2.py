import numpy as np
import matplotlib.pyplot as plt
import os
from scipy.linalg import svd

from sklearn import linear_model
from sklearn.linear_model import LinearRegression
from sklearn.linear_model import Ridge, Lasso

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
                  test_x_time_mean,test_x_time_var,test_x_rssi_mean,test_x_rssi_var]
    
    return packet

def DMD(train_x):
    X = train_x[:, :-1]  # 输入数据矩阵
    Y = train_x[:, 1:]   # 输出数据矩阵

    U, S, V = svd(X, full_matrices=False)  # 奇异值分解

    Ur = U[:, :]     # 选择前 r 个奇异向量
    Sr = np.diag(S[:])  # 选择前 r 个奇异值
    Vr = V.conj().T[:, :]  # 选择前 r 个奇异向量的共轭转置

    Atilde = Ur.conj().T @ Y @ Vr @ np.linalg.inv(Sr)  # 构建系统矩阵 Atilde

    eigenvalues, W = np.linalg.eig(Atilde)  # 计算特征值和特征向量

    Phi = Y @ Vr @ np.linalg.inv(Sr) @ W  # 构建 DMD 模态矩阵 Phi

    return Phi, eigenvalues



def main():
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

    packet = build_trainset_and_testset(train_data,test_data,5,500)

    train_x = np.array(packet[0])
    train_y = np.array(packet[1])
    test_x = np.array(packet[2])
    test_y = np.array(packet[3])
    train_x_time_mean = np.array(packet[4])
    train_x_time_var = np.array(packet[5])
    train_x_rssi_mean = np.array(packet[6])
    train_x_rssi_var = np.array(packet[7])

    test_x_time_mean = np.array(packet[8])
    test_x_time_var = np.array(packet[9])
    test_x_rssi_mean = np.array(packet[10])
    test_x_rssi_var = np.array(packet[11])
    '''
    train_x_time_mean = np.array(train_x_time_mean)
    train_x_time_var = np.array(train_x_time_var)
    train_x_rssi_mean = np.array(train_x_rssi_mean)
    train_x_rssi_var = np.array(train_x_rssi_var)

    train_x = np.array(train_x)
    train_y = np.array(train_y)
    test_x = np.array(test_x)
    test_y = np.array(test_y)
    '''
    train_PHi_list = []
    train_eigenvalues_list = []
    for i in range(len(train_x)):
        PHi,eigenvalues = DMD(train_x[i])
        train_PHi_list.append(PHi)
        train_eigenvalues_list.append(eigenvalues)

    train_PHi_array = np.array(train_PHi_list)
    train_eigenvalues_array = np.array(train_eigenvalues_list)      #(80,2)         80 samples,each sample have 2 DMD features

    train_merged_array = np.concatenate((train_x_time_mean[:, np.newaxis],train_x_time_var[:, np.newaxis],train_x_rssi_mean[:, np.newaxis],train_x_rssi_var[:, np.newaxis]), axis=1)

    X = train_merged_array
    Y = train_y

    lin_model = LinearRegression()
    lin_model.fit(X, Y)

    ridge_model = Ridge()
    ridge_model.fit(X,Y)

    lasso_model = Lasso()
    lasso_model.fit(X,Y)

    test_PHi_list = []
    test_eigenvalues_list = []
    for i in range(len(test_x)):
        PHi,eigenvalues = DMD(test_x[i])
        test_PHi_list.append(PHi)
        test_eigenvalues_list.append(eigenvalues)

    test_PHi_array = np.array(test_PHi_list)
    test_eigenvalues_array = np.array(test_eigenvalues_list)

    test_merged_array = np.concatenate((test_x_time_mean[:, np.newaxis],test_x_time_var[:, np.newaxis],test_x_rssi_mean[:, np.newaxis],test_x_rssi_var[:, np.newaxis]), axis=1)

    test_x = test_merged_array

    lin_predictions = lin_model.predict(test_x)
    ridge_predictions = ridge_model.predict(test_x)
    lasso_predictions = lasso_model.predict(test_x)


    plt.figure()
    plt.scatter([i for i in range(len(test_x))],lasso_predictions, c = 'b',label = 'lasso_predictions')
    plt.plot([i for i in range(len(test_x))],[i for i in range(len(test_x))],c = 'r',label = 'true value')
    plt.legend()
    plt.show()


    

main()
