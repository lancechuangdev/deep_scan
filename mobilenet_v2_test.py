import os
import shutil
import argparse
import numpy as np
from tensorflow.keras.models import load_model
from tensorflow.keras.preprocessing.image import load_img, img_to_array

model_path = "/home/liang/Documents/notebook/mobilenet/mobilenetv2_fabric_anomaly_grayscale.h5"

def preprocess_image(image_path, target_size=(256, 256)):
    # Load the image in grayscale mode
    img = load_img(image_path, target_size=target_size, color_mode='grayscale')
    
    # Convert to numpy array
    img_array = img_to_array(img)
    
    # Convert grayscale to RGB
    img_rgb = np.stack((img_array[..., 0],) * 3, axis=-1)  # (256, 256, 3)
    
    # Normalize pixel values
    img_rgb = img_rgb / 255.0
    
    # Add batch dimension
    img_rgb = np.expand_dims(img_rgb, axis=0)  # (1, 256, 256, 3)
    
    return img_rgb

def main():
    parser = argparse.ArgumentParser(description='Classifer images to normal and anomaly.')
    parser.add_argument('--images_dir', type=str, required=True, help='Path to the images directory')
    
    args = parser.parse_args()
    images_dir = args.images_dir

    # Create directories for classified images if they don't exist
    anomaly_dir = os.path.join(images_dir, 'anomaly')
    normal_dir = os.path.join(images_dir, 'normal')
    os.makedirs(anomaly_dir, exist_ok=True)
    os.makedirs(normal_dir, exist_ok=True)

    # Load the saved model
    model = load_model(model_path)

    # Iterate over all images
    for file_name in os.listdir(images_dir):
        if file_name.endswith('.jpg') or file_name.endswith('.png'):
            image_path = os.path.join(images_dir, file_name)

            # Skip already classified images
            if os.path.dirname(image_path) in [anomaly_dir, normal_dir]:
                continue

            # Preprocess the image
            preprocessed_image = preprocess_image(image_path)
            
            # Predict
            prediction = model.predict(preprocessed_image)
            
            # Display results
            print(f"Image: {file_name}, Prediction: {prediction[0]}")
            if prediction[0] > 0.5:
                shutil.move(image_path, os.path.join(anomaly_dir, file_name))
                print(f"Moved {file_name} to 'anomaly' folder.")
            else:
                shutil.move(image_path, os.path.join(normal_dir, file_name))
                print(f"Moved {file_name} to 'normal' folder.")

if __name__ == "__main__":
    main()
