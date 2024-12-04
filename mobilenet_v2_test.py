import os
import shutil
import argparse
from PIL import Image
import numpy as np
from tensorflow.keras.models import load_model

model_path = '/usr/local/share/deep-scan/ee.h5'
patch_size = 256

def preprocess_image(image_path):
    """Preprocess the image for the model."""
    # Read the image in grayscale
    image = Image.open(image_path).convert('L')  # 'L' mode converts to grayscale
    # Normalize pixel values to [0, 1]
    image = np.array(image, dtype='float32') / 255.0
    return image

def build_batch(image, patch_size):
    batch = []
    height, width = image.shape[:2]

    # Extract and reshape each patch_size*patch_size patch
    num_patches_x = width // patch_size
    print(f"num_patches_x: {num_patches_x}")
    num_patches_y = height // patch_size
    print(f"num_patches_y: {num_patches_y}")

    for j in range(num_patches_y):
        for i in range(num_patches_x):
            patch = image[j * patch_size:(j + 1) * patch_size, i * patch_size:(i + 1) * patch_size]
            patch_rgb = np.stack([patch, patch, patch], axis=-1)  # Convert to RGB
            batch.append(patch_rgb)

        # Handle the remaining part as a smaller patch, if any
        remainder = width % patch_size

        if remainder > 0:
            # Adjust the start position for the last patch so it aligns properly
            start_x = width - patch_size
            last_patch = image[j * patch_size:(j + 1) * patch_size, start_x:start_x + patch_size]
            last_patch_rgb = np.stack([last_patch, last_patch, last_patch], axis=-1)
            batch.append(last_patch_rgb)

    return np.array(batch)  # Convert to NumPy array

def main():
    parser = argparse.ArgumentParser(description='Classifer images to normal and anomaly.')
    parser.add_argument('--images_dir', type=str, required=True, help='Path to the images directory')
    parser.add_argument('--threshold', type=float, required=True, help='Threshold for binary classification')

    args = parser.parse_args()
    images_dir = args.images_dir
    threshold = args.threshold

    # Create directories for classified images if they don't exist
    anomaly_dir = os.path.join(images_dir, 'anomaly')
    normal_dir = os.path.join(images_dir, 'normal')
    os.makedirs(anomaly_dir, exist_ok=True)
    os.makedirs(normal_dir, exist_ok=True)

    # Load the saved model
    model = load_model(model_path)

    # Iterate over all images
    for file_name in os.listdir(images_dir):
        if file_name.endswith(('.jpg', '.png', '.bmp')):
            image_path = os.path.join(images_dir, file_name)

            # Skip already classified images
            if os.path.dirname(image_path) in [anomaly_dir, normal_dir]:
                continue
            
            # Preprocess the image
            image = preprocess_image(image_path)

            # Build a batch of patches
            batch = build_batch(image, patch_size)

            # Predict
            predictions = model.predict(batch)
            anomaly_patches = np.sum(predictions > threshold)  # Count anomaly patches

            print(f"Image: {file_name}, Total Patches: {len(predictions)}, Anomaly Patches: {anomaly_patches}")
            if anomaly_patches >= 1:
                shutil.move(image_path, os.path.join(anomaly_dir, file_name))
                print(f"Moved {file_name} to 'anomaly' folder.")
            else:
                shutil.move(image_path, os.path.join(normal_dir, file_name))
                print(f"Moved {file_name} to 'normal' folder.")

if __name__ == "__main__":
    main()
