import numpy as np
import matplotlib.pyplot as plt
from skimage.transform import resize
from sklearn.decomposition import PCA

# load data

def Load(file):
    data = np.loadtxt(file)

    print("Shape:", data.shape)
    print("Min/Max:", data.min(), data.max())

    plt.imshow(data, cmap='gray')
    plt.title('Reconstructed Frame')
    plt.colorbar()
    plt.show()

    small = resize(data, (8, 8), anti_aliasing=True)

    plt.figure(figsize=(8, 3))

    plt.subplot(1, 2, 1)
    plt.imshow(data, cmap='gray')
    plt.title("Original")

    plt.subplot(1, 2, 2)
    plt.imshow(small, cmap='gray')
    plt.title("8x8 Downsample")

    plt.show()

    #try pca


    n_components = 40
    pca = PCA(n_components=n_components)

    reduced = pca.fit_transform(data)
    reconstructed = pca.inverse_transform(reduced)

    mse = np.mean((data - reconstructed) ** 2)
    explained = pca.explained_variance_ratio_.sum()

    print("Reduced shape:", reduced.shape)
    print("Explained variance:", explained)
    print("Reconstruction MSE:", mse)

    plt.figure(figsize=(12, 4))

    plt.subplot(1, 3, 1)
    plt.imshow(data, cmap='gray')
    plt.title("Original")

    plt.subplot(1, 3, 2)
    plt.imshow(reconstructed, cmap='gray')
    plt.title(f"PCA Reconstructed ({n_components})")

    plt.subplot(1, 3, 3)
    plt.imshow(np.abs(data - reconstructed), cmap='hot')
    plt.title("Absolute Error")

    plt.show()
    
    ## values

    mean_brightness = data.mean()
    min_val = data.min()
    max_val = data.max()
    std_brightness = data.std()

    print("Mean brightness:", mean_brightness)
    print("Min:", min_val)
    print("Max:", max_val)
    print("Std dev:", std_brightness)

    h, w = data.shape
    top = data[:h//2, :].mean()
    bottom = data[h//2:, :].mean()
    left = data[:, :w//2].mean()
    right = data[:, w//2:].mean()

    print("Top:", top)
    print("Bottom:", bottom)
    print("Left:", left)
    print("Right:", right)


    ##calculate avg 
    avg_brightness = data.mean()
    print ("Average brightness:", avg_brightness)

## 1 for downsample, the other for averages 

Load('face_gray.txt')

def average(file):
    ## values

    data = np.load(file)
    mean_brightness = data.mean()
    min_val = data.min()
    max_val = data.max()
    std_brightness = data.std()

    print("Mean brightness:", mean_brightness)
    print("Min:", min_val)
    print("Max:", max_val)
    print("Std dev:", std_brightness)

    h, w = data.shape
    top = data[:h//2, :].mean()
    bottom = data[h//2:, :].mean()
    left = data[:, :w//2].mean()
    right = data[:, w//2:].mean()

    print("Top:", top)
    print("Bottom:", bottom)
    print("Left:", left)
    print("Right:", right)


    ##calculate avg 
    avg_brightness = data.mean()
    print ("Average brightness:", avg_brightness)

    //output is csv sent to task c 