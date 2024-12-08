#include "pyscript.h"

const std::string unet_16to256 = R"(
import os
import argparse
import tensorflow as tf
import matplotlib.pyplot as plt

def augment_image(image, mask):
    # Random horizontal flip
    image = tf.image.random_flip_left_right(image)
    mask = tf.image.random_flip_left_right(mask)

    # Random vertical flip
    image = tf.image.random_flip_up_down(image)
    mask = tf.image.random_flip_up_down(mask)

    # Random brightness
    image = tf.image.random_brightness(image, max_delta=0.1)

    # Random contrast
    image = tf.image.random_contrast(image, lower=0.9, upper=1.1)

    # Randomly rotate by 0, 90, 180, or 270 degrees
    k = tf.random.uniform([], minval=0, maxval=4, dtype=tf.int32)
    image = tf.image.rot90(image, k=k)
    mask = tf.image.rot90(mask, k=k)

    return image, mask

def load_images_and_masks(image_dir, mask_dir, target_size=(256, 256), batch_size=8, normalize=True, augment=False):
    # Load images
    image_dataset = tf.keras.utils.image_dataset_from_directory(
        image_dir,
        labels=None,
        image_size=target_size,
        batch_size=batch_size,
        color_mode='grayscale',
        shuffle=False
    )

    # Load masks
    mask_dataset = tf.keras.utils.image_dataset_from_directory(
        mask_dir,
        labels=None,
        image_size=target_size,
        batch_size=batch_size,
        color_mode='grayscale',
        shuffle=False
    )

    # Pair images with their masks
    dataset = tf.data.Dataset.zip((image_dataset, mask_dataset))

    # Augment the dataset
    if augment:
        dataset = dataset.map(augment_image)

    for img_batch, mask_batch in dataset.take(1):
        print("Before Normalization:")
        print(f"Image min value: {tf.reduce_min(img_batch).numpy()}, max value: {tf.reduce_max(img_batch).numpy()}")
        print(f"Mask min value: {tf.reduce_min(mask_batch).numpy()}, max value: {tf.reduce_max(mask_batch).numpy()}")

    # Normalize the images and masks to [0, 1]
    if normalize:
        dataset = dataset.map(lambda img, mask: (tf.image.convert_image_dtype(img, tf.float32) / 255.0,
                                                 tf.image.convert_image_dtype(mask, tf.float32) / 255.0))

    for img_batch, mask_batch in dataset.take(1):
        print("After Normalization:")
        print(f"Image min value: {tf.reduce_min(img_batch).numpy()}, max value: {tf.reduce_max(img_batch).numpy()}")
        print(f"Mask min value: {tf.reduce_min(mask_batch).numpy()}, max value: {tf.reduce_max(mask_batch).numpy()}")

    return dataset

# U-Net model definition
def unet_model(input_size):
    inputs = tf.keras.layers.Input(input_size)

    # Contraction path
    filters = [16, 32, 64, 128, 256]
    dropouts = [0.1, 0.1, 0.2, 0.2, 0.3]
    pool_size = (2, 2)

    skip_connections = []  # To store the connections for the expansive path

    x = inputs  # Initial input to the network

    # Downsampling (contraction) loop
    for f, d in zip(filters[:-1], dropouts[:-1]):
        x = tf.keras.layers.Conv2D(f, (3, 3), activation='relu', kernel_initializer='he_normal', padding='same')(x)
        x = tf.keras.layers.Dropout(d)(x)
        x = tf.keras.layers.Conv2D(f, (3, 3), activation='relu', kernel_initializer='he_normal', padding='same')(x)
        x = tf.keras.layers.BatchNormalization()(x)
        x = tf.keras.layers.ReLU()(x)

        # Store skip connection for the upsampling path
        skip_connections.append(x)

        x = tf.keras.layers.MaxPooling2D(pool_size)(x)

    # Bottleneck
    x = tf.keras.layers.Conv2D(filters[-1], (3, 3), activation='relu', kernel_initializer='he_normal', padding='same')(x)
    x = tf.keras.layers.BatchNormalization()(x)
    x = tf.keras.layers.ReLU()(x)
    x = tf.keras.layers.Dropout(dropouts[-1])(x)
    x = tf.keras.layers.Conv2D(filters[-1], (3, 3), activation='relu', kernel_initializer='he_normal', padding='same')(x)

    # Upsampling (expansive) path
    for f, skip in zip(reversed(filters[:-1]), reversed(skip_connections)):
        x = tf.keras.layers.Conv2DTranspose(f, (2, 2), strides=(2, 2), padding='same')(x)
        x = tf.keras.layers.concatenate([x, skip])  # Skip connection
        x = tf.keras.layers.BatchNormalization()(x)
        x = tf.keras.layers.ReLU()(x)

    # Final output layer
    outputs = tf.keras.layers.Conv2D(1, (1, 1), activation='sigmoid')(x)

    model = tf.keras.models.Model(inputs=[inputs], outputs=[outputs])
    return model

# BCE w/ Intersection over Union (IoU)
def iou(y_true, y_pred):
    y_pred = tf.round(y_pred)
    intersection = tf.reduce_sum(y_true * y_pred)
    total = tf.reduce_sum(y_true + y_pred)
    union = total - intersection
    iou = intersection / (union + tf.keras.backend.epsilon())
    return iou

def main():
    # Set up argument parser
    parser = argparse.ArgumentParser(description='Train the UNet model on images and masks.')
    parser.add_argument('--model_path', type=str, required=True, help='Path to save the trained model file')
    parser.add_argument('--train_images_path', type=str, required=True, help='Path to the training images directory')
    parser.add_argument('--train_masks_path', type=str, required=True, help='Path to the training masks directory')
    parser.add_argument('--val_images_path', type=str, required=True, help='Path to the validation images directory')
    parser.add_argument('--val_masks_path', type=str, required=True, help='Path to the validation masks directory')
    parser.add_argument('--patch_size', type=int, required=True, help='Patch size for model')
    parser.add_argument('--batch_size', type=int, required=True, help='Batch size for training')
    parser.add_argument('--epochs', type=int, required=True, help='The number of complete passes through the entire training dataset')
    
    args = parser.parse_args()

    model_path = args.model_path
    train_images_path = args.train_images_path
    train_masks_path = args.train_masks_path
    val_images_path = args.val_images_path
    val_masks_path = args.val_masks_path
    patch_size = args.patch_size
    batch_size = args.batch_size
    epochs = args.epochs

    # Load datasets
    train_dataset = load_images_and_masks(train_images_path, train_masks_path, target_size=(patch_size, patch_size), batch_size=batch_size)
    val_dataset = load_images_and_masks(val_images_path, val_masks_path, target_size=(patch_size, patch_size), batch_size=batch_size)

    # U-net model
    model = unet_model(input_size=(patch_size, patch_size, 1))

    # Train the model
    model.compile(optimizer='adam', loss='binary_crossentropy', metrics=[iou]) # metrics=['accuracy', iou_metric]?
    checkpoint_callback = tf.keras.callbacks.ModelCheckpoint(model_path, save_best_only=True)
    history = model.fit(train_dataset, epochs=epochs, validation_data=val_dataset, callbacks=[checkpoint_callback])

    # Create a figure with subplots
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(20, 5))

    # Plotting loss
    ax1.plot(history.history['loss'], label='Train loss')
    ax1.plot(history.history['val_loss'], label='Val loss')
    ax1.set_xlabel('Epochs')
    ax1.set_ylabel('Loss')
    ax1.legend()

    # Plotting IOU
    ax2.plot(history.history['iou'], label='Train IOU')
    ax2.plot(history.history['val_iou'], label='Val IOU')
    ax2.set_xlabel('Epochs')
    ax2.set_ylabel('IOU')
    ax2.legend()

    # Save the plot to an image file
    dir = os.path.dirname(model_path)
    metrics = 'training_metrics.png'
    plt.savefig(os.path.join(dir, metrics))

if __name__ == "__main__":
    main()
)";

const std::string unet_16to512 = R"(
import os
import argparse
import tensorflow as tf
import matplotlib.pyplot as plt

def augment_image(image, mask):
    # Random horizontal flip
    image = tf.image.random_flip_left_right(image)
    mask = tf.image.random_flip_left_right(mask)

    # Random vertical flip
    image = tf.image.random_flip_up_down(image)
    mask = tf.image.random_flip_up_down(mask)

    # Random brightness
    image = tf.image.random_brightness(image, max_delta=0.1)

    # Random contrast
    image = tf.image.random_contrast(image, lower=0.9, upper=1.1)

    # Randomly rotate by 0, 90, 180, or 270 degrees
    k = tf.random.uniform([], minval=0, maxval=4, dtype=tf.int32)
    image = tf.image.rot90(image, k=k)
    mask = tf.image.rot90(mask, k=k)

    return image, mask

def load_images_and_masks(image_dir, mask_dir, target_size=(256, 256), batch_size=8, normalize=True, augment=False):
    # Load images
    image_dataset = tf.keras.utils.image_dataset_from_directory(
        image_dir,
        labels=None,
        image_size=target_size,
        batch_size=batch_size,
        color_mode='grayscale',
        shuffle=False
    )

    # Load masks
    mask_dataset = tf.keras.utils.image_dataset_from_directory(
        mask_dir,
        labels=None,
        image_size=target_size,
        batch_size=batch_size,
        color_mode='grayscale',
        shuffle=False
    )

    # Pair images with their masks
    dataset = tf.data.Dataset.zip((image_dataset, mask_dataset))

    # Augment the dataset
    if augment:
        dataset = dataset.map(augment_image)

    for img_batch, mask_batch in dataset.take(1):
        print("Before Normalization:")
        print(f"Image min value: {tf.reduce_min(img_batch).numpy()}, max value: {tf.reduce_max(img_batch).numpy()}")
        print(f"Mask min value: {tf.reduce_min(mask_batch).numpy()}, max value: {tf.reduce_max(mask_batch).numpy()}")

    # Normalize the images and masks to [0, 1]
    if normalize:
        dataset = dataset.map(lambda img, mask: (tf.image.convert_image_dtype(img, tf.float32) / 255.0,
                                                 tf.image.convert_image_dtype(mask, tf.float32) / 255.0))

    for img_batch, mask_batch in dataset.take(1):
        print("After Normalization:")
        print(f"Image min value: {tf.reduce_min(img_batch).numpy()}, max value: {tf.reduce_max(img_batch).numpy()}")
        print(f"Mask min value: {tf.reduce_min(mask_batch).numpy()}, max value: {tf.reduce_max(mask_batch).numpy()}")

    return dataset

# U-Net model definition
def unet_model(input_size):
    inputs = tf.keras.layers.Input(input_size)

    # Contraction path
    filters = [16, 32, 64, 128, 256, 512]
    dropouts = [0.1, 0.1, 0.2, 0.2, 0.3, 0.3]
    pool_size = (2, 2)

    skip_connections = []  # To store the connections for the expansive path

    x = inputs  # Initial input to the network

    # Downsampling (contraction) loop
    for f, d in zip(filters[:-1], dropouts[:-1]):
        x = tf.keras.layers.Conv2D(f, (3, 3), activation='relu', kernel_initializer='he_normal', padding='same')(x)
        x = tf.keras.layers.Dropout(d)(x)
        x = tf.keras.layers.Conv2D(f, (3, 3), activation='relu', kernel_initializer='he_normal', padding='same')(x)
        x = tf.keras.layers.BatchNormalization()(x)
        x = tf.keras.layers.ReLU()(x)

        # Store skip connection for the upsampling path
        skip_connections.append(x)

        x = tf.keras.layers.MaxPooling2D(pool_size)(x)

    # Bottleneck
    x = tf.keras.layers.Conv2D(filters[-1], (3, 3), activation='relu', kernel_initializer='he_normal', padding='same')(x)
    x = tf.keras.layers.BatchNormalization()(x)
    x = tf.keras.layers.ReLU()(x)
    x = tf.keras.layers.Dropout(dropouts[-1])(x)
    x = tf.keras.layers.Conv2D(filters[-1], (3, 3), activation='relu', kernel_initializer='he_normal', padding='same')(x)

    # Upsampling (expansive) path
    for f, skip in zip(reversed(filters[:-1]), reversed(skip_connections)):
        x = tf.keras.layers.Conv2DTranspose(f, (2, 2), strides=(2, 2), padding='same')(x)
        x = tf.keras.layers.concatenate([x, skip])  # Skip connection
        x = tf.keras.layers.BatchNormalization()(x)
        x = tf.keras.layers.ReLU()(x)

    # Final output layer
    outputs = tf.keras.layers.Conv2D(1, (1, 1), activation='sigmoid')(x)

    model = tf.keras.models.Model(inputs=[inputs], outputs=[outputs])
    return model

# BCE w/ Intersection over Union (IoU)
def iou(y_true, y_pred):
    y_pred = tf.round(y_pred)
    intersection = tf.reduce_sum(y_true * y_pred)
    total = tf.reduce_sum(y_true + y_pred)
    union = total - intersection
    iou = intersection / (union + tf.keras.backend.epsilon())
    return iou

def main():
    # Set up argument parser
    parser = argparse.ArgumentParser(description='Train the UNet model on images and masks.')
    parser.add_argument('--model_path', type=str, required=True, help='Path to save the trained model file')
    parser.add_argument('--train_images_path', type=str, required=True, help='Path to the training images directory')
    parser.add_argument('--train_masks_path', type=str, required=True, help='Path to the training masks directory')
    parser.add_argument('--val_images_path', type=str, required=True, help='Path to the validation images directory')
    parser.add_argument('--val_masks_path', type=str, required=True, help='Path to the validation masks directory')
    parser.add_argument('--patch_size', type=int, required=True, help='Patch size for model')
    parser.add_argument('--batch_size', type=int, required=True, help='Batch size for training')
    parser.add_argument('--epochs', type=int, required=True, help='The number of complete passes through the entire training dataset')
    
    args = parser.parse_args()

    model_path = args.model_path
    train_images_path = args.train_images_path
    train_masks_path = args.train_masks_path
    val_images_path = args.val_images_path
    val_masks_path = args.val_masks_path
    patch_size = args.patch_size
    batch_size = args.batch_size
    epochs = args.epochs

    # Load datasets
    train_dataset = load_images_and_masks(train_images_path, train_masks_path, target_size=(patch_size, patch_size), batch_size=batch_size)
    val_dataset = load_images_and_masks(val_images_path, val_masks_path, target_size=(patch_size, patch_size), batch_size=batch_size)

    # U-net model
    model = unet_model(input_size=(patch_size, patch_size, 1))

    # Train the model
    model.compile(optimizer='adam', loss='binary_crossentropy', metrics=[iou]) # metrics=['accuracy', iou_metric]?
    checkpoint_callback = tf.keras.callbacks.ModelCheckpoint(model_path, save_best_only=True)
    history = model.fit(train_dataset, epochs=epochs, validation_data=val_dataset, callbacks=[checkpoint_callback])

    # Create a figure with subplots
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(20, 5))

    # Plotting loss
    ax1.plot(history.history['loss'], label='Train loss')
    ax1.plot(history.history['val_loss'], label='Val loss')
    ax1.set_xlabel('Epochs')
    ax1.set_ylabel('Loss')
    ax1.legend()

    # Plotting IOU
    ax2.plot(history.history['iou'], label='Train IOU')
    ax2.plot(history.history['val_iou'], label='Val IOU')
    ax2.set_xlabel('Epochs')
    ax2.set_ylabel('IOU')
    ax2.legend()

    # Save the plot to an image file
    dir = os.path.dirname(model_path)
    metrics = 'training_metrics.png'
    plt.savefig(os.path.join(dir, metrics))

if __name__ == "__main__":
    main()
)";

const std::string unet_test_py = R"(
import os
import argparse
import numpy as np
import math
import tensorflow as tf
from PIL import Image

def load_images_and_masks(image_dir, mask_dir, target_size=(256, 256), batch_size=8):
    # Load images
    image_dataset = tf.keras.utils.image_dataset_from_directory(
        image_dir,
        labels=None,
        image_size=target_size,
        batch_size=batch_size,
        color_mode='grayscale',
        shuffle=False
    )

    # Load masks
    mask_dataset = tf.keras.utils.image_dataset_from_directory(
        mask_dir,
        labels=None,
        image_size=target_size,
        batch_size=batch_size,
        color_mode='grayscale',
        shuffle=False
    )

    # Pair images with their masks
    dataset = tf.data.Dataset.zip((image_dataset, mask_dataset))

    # Normalize the images to [0, 1], required by the unet model.
    # Dont normalize the masks, keep them in the range of [0, 255] to display it
    dataset = dataset.map(lambda img, mask: (tf.image.convert_image_dtype(img, tf.float32) / 255.0,
                                             tf.image.convert_image_dtype(mask, tf.float32)))

    return dataset

# BCE w/ Intersection over Union (IoU)
def iou(y_true, y_pred):
    y_pred = tf.round(y_pred)
    intersection = tf.reduce_sum(y_true * y_pred)
    total = tf.reduce_sum(y_true + y_pred)
    union = total - intersection
    iou = intersection / (union + tf.keras.backend.epsilon())
    return iou

def main():
    # Set up argument parser
    parser = argparse.ArgumentParser(description='Run the UNet model on test images.')
    parser.add_argument('--model_path', type=str, required=True, help='Path to the trained model file')
    parser.add_argument('--test_images_path', type=str, required=True, help='Path to the test images directory')
    parser.add_argument('--test_masks_path', type=str, required=True, help='Path to the test masks directory')
    parser.add_argument('--patch_size', type=int, required=True, help='Patch size for model')
    parser.add_argument('--batch_size', type=int, required=True, help='Batch size for training')
    parser.add_argument('--threshold', type=float, required=True, help='Threshold for mask prediction')
    
    args = parser.parse_args()

    model_path = args.model_path
    test_images_path = args.test_images_path
    test_masks_path = args.test_masks_path
    patch_size = args.patch_size
    batch_size = args.batch_size
    threshold = args.threshold

    # Load the test dataset
    test_dataset = load_images_and_masks(test_images_path, test_masks_path,target_size=(patch_size, patch_size), batch_size=batch_size)

    # Define the custom objects dictionary
    custom_objects = {
        'iou': iou
    }
    loaded_model = tf.keras.models.load_model(model_path, custom_objects=custom_objects)

    patches_per_row = 4
    rows = math.ceil(batch_size / patches_per_row)

    # Size of the stitched image
    spacing=5
    border_thickness=5
    stitched_height = rows * (patch_size + 2 * border_thickness + spacing) - spacing
    stitched_width = patches_per_row * (patch_size + 2 * border_thickness + spacing) - spacing
    stride = patch_size + 2 * border_thickness + spacing
    
    model_dir = os.path.dirname(model_path)

    for idx, (images, masks) in enumerate(test_dataset):
        # Convert images to uint8 format by scaling them to [0, 255] for proper display
        images_display = (images.numpy() * 255).astype(np.uint8)

        # Predict masks
        preds = loaded_model.predict(images)
        preds = (preds > threshold).astype(np.float32)

        stitched_image = np.zeros((stitched_height, stitched_width, 3), dtype=np.uint8)
        stitched_true_mask = np.zeros((stitched_height, stitched_width), dtype=np.float32)
        stitched_pred_mask = np.zeros((stitched_height, stitched_width), dtype=np.float32)

        patch_idx = 0
        # Stitch patches
        for i in range(0, stitched_height, stride):
            for j in range(0, stitched_width, stride):
                if (patch_idx >= len(images)):
                    break

                # Place the patches in their respective positions
                patch_with_border = np.pad(images_display[patch_idx], ((border_thickness, border_thickness),
                                                               (border_thickness, border_thickness),
                                                               (0, 0)), mode='constant', constant_values=128)
                stitched_image[i:i + patch_with_border.shape[0], j:j + patch_with_border.shape[1]] = patch_with_border
                
                # Place true masks with border
                true_mask_with_border = np.pad(np.squeeze(masks[patch_idx]), ((border_thickness, border_thickness),
                                                                              (border_thickness, border_thickness)),
                                                                              mode='constant', constant_values=128)
                stitched_true_mask[i:i + true_mask_with_border.shape[0], j:j + true_mask_with_border.shape[1]] = true_mask_with_border
        
                # Place predicted masks with border
                pred_mask_with_border = np.pad(np.squeeze(preds[patch_idx]), ((border_thickness, border_thickness),
                                                                              (border_thickness, border_thickness)),
                                                                              mode='constant', constant_values=128)
                stitched_pred_mask[i:i + pred_mask_with_border.shape[0], j:j + pred_mask_with_border.shape[1]] = pred_mask_with_border
        
                # Move to the next patch
                patch_idx += 1

        # Normalize predicted masks to [0, 255] range for saving
        stitched_pred_mask = (stitched_pred_mask * 255).astype(np.uint8)

        # Convert grayscale masks to RGB by stacking them (height, width -> height, width, 3)
        stitched_true_mask_rgb = np.stack([stitched_true_mask] * 3, axis=-1)  # Convert to 3-channel image
        stitched_pred_mask_rgb = np.stack([stitched_pred_mask] * 3, axis=-1)  # Convert to 3-channel image

        # Define space between the stitched image and masks
        space_size = 20  # This can be any pixel value you want for spacing
        space_color = 0  # Black space

        # Create blank space for separating the images
        # For RGB images (3 channels)
        black_space_rgb = np.ones((stitched_height, space_size, 3), dtype=np.uint8) * space_color

        # Concatenate the image, true mask, and predicted mask horizontally
        combined_image = np.concatenate((stitched_image, black_space_rgb, stitched_true_mask_rgb, black_space_rgb, stitched_pred_mask_rgb), axis=1)

        # Convert the combined image to uint8
        combined_image = combined_image.astype(np.uint8)

        # Convert the combined image to a PIL image and save it
        combined_pil_image = Image.fromarray(combined_image)
        os.makedirs(os.path.join(model_dir, 'test_result'), exist_ok=True)
        combined_pil_image.save(os.path.join(model_dir, 'test_result', f'stitched_result_{idx}.png'))

        print(f'Saved combined image for test sample batch {idx}')

if __name__ == "__main__":
    main()
)";

const std::string mobilenet_v2_test = R"(
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
)";

const std::string data_augment = R"(
import os
import argparse
import shutil
import cv2
import numpy as np
import albumentations as A

def organize_files(source_dir, images_dir, masks_dir):
    """
    Organizes files in the source directory into 'images' and 'masks' folders.
    
    Args:
        source_dir (str): Path to the directory containing the files.
    """
    
    # Create the 'images' and 'masks' folders
    os.makedirs(images_dir, exist_ok=True)
    os.makedirs(masks_dir, exist_ok=True)
    
    # Iterate over all files in the source directory
    for file_name in os.listdir(source_dir):
        file_path = os.path.join(source_dir, file_name)
        
        # Check if it's a file
        if os.path.isfile(file_path):
            if file_name.endswith("_mask.bmp"):
                # Move mask files to the 'masks' folder and rename them
                new_name = file_name.replace("_mask", "")
                shutil.move(file_path, os.path.join(masks_dir, new_name))
            elif file_name.endswith(".bmp"):
                # Move regular images to the 'images' folder
                shutil.move(file_path, os.path.join(images_dir, file_name))
    
    print(f"Files organized successfully into '{images_dir}' and '{masks_dir}'.")

def load_image_mask_pairs(images_dir, masks_dir):
    """
    Load paired images and masks from the given directories and return image-mask pairs with filenames.

    Args:
        images_dir (str): Path to the folder containing the images.
        masks_dir (str): Path to the folder containing the masks.

    Returns:
        list of tuples: A list where each element is a tuple (image, mask, filename).
    """
    image_mask_pairs = []

    # Ensure the images and masks are paired
    image_files = sorted(os.listdir(images_dir))  # Sort to ensure pairing
    mask_files = sorted(os.listdir(masks_dir))   # Sort to ensure pairing

    for image_file, mask_file in zip(image_files, mask_files):
        # Ensure the names match (without extensions)
        if os.path.splitext(image_file)[0] == os.path.splitext(mask_file)[0]:
            # Load image and mask
            image_path = os.path.join(images_dir, image_file)
            mask_path = os.path.join(masks_dir, mask_file)

            image = cv2.imread(image_path)
            mask = cv2.imread(mask_path)

            if image is not None and mask is not None:
                # Store the pair along with the filename (without extension)
                filename = os.path.splitext(image_file)[0]  # Extract base filename without extension
                image_mask_pairs.append((image, mask, filename))
            else:
                print(f"Could not load image or mask: {image_path}, {mask_path}")
        else:
            print(f"File name mismatch: {image_file} and {mask_file}")

    return image_mask_pairs

def extract_patches(image, patch_size, stride):
    """
    Extract patches from the image, ensuring all patches fit within the image without padding.
    
    Parameters:
        image (np.array): The large input image.
        patch_size (int): Size of the patches.
        stride (int): Stride for the sliding window.

    Returns:
        List of patches as numpy arrays.
    """
    height, width, _ = image.shape
    patches = []
    print(f"height: {height}, width: {width}")
    
    # Loop through rows
    y = 0
    while y < height:
        print(f"y: {y}")
        # Break out of the loop for the last row patch
        if y + patch_size >= height:
            break
        # Loop through columns
        x = 0
        while x < width:
            print(f"x: {x}")
            # Break out of the loop for the last column patch
            if x + patch_size >= width:
                break  
            # Extract the patch
            patch = image[y:y + patch_size, x:x + patch_size]
            patches.append(patch)
            # Increment by stride
            x += stride
        # Increment by stride
        y += stride

    return patches

def save_patches(patches, output_dir, prefix="patch"):
    """
    Save patches to files and display them in a grid.

    Args:
        patches (list of numpy arrays): List of image patches.
        output_dir (str): Directory to save the patches.
        prefix (str): Prefix for patch filenames.
    """
    # Ensure the output directory exists
    os.makedirs(output_dir, exist_ok=True)

    for i, patch in enumerate(patches):
        # Save each patch
        filename = os.path.join(output_dir, f"{prefix}_{i + 1:03}.png")
        # OpenCV assumes the array is in BGR format for color images or grayscale for single channel
        cv2.imwrite(filename, patch)

def augment_image_and_mask(image, mask, patch_size=(512, 512)):
    # Define augmentation pipeline
    transform = A.Compose(
        [
            # Spatial Transformations (applies to both image and mask)
            A.ShiftScaleRotate(shift_limit=0.2, scale_limit=0.1, rotate_limit=5, border_mode=1),
            A.HorizontalFlip(p=0.5),
            A.VerticalFlip(p=0.5),

            # Brightness and Contrast (applies only to the image)
            A.OneOf(
                [
                    A.RandomBrightnessContrast(brightness_limit=(-0.15, 0.05), contrast_limit=0.15, p=0.5),
                ],
                p=0.5,  # Apply brightness/contrast with a probability
            ),
        ],
        additional_targets={"mask": "mask"},  # Ensure the mask is treated separately
    )

    # Apply augmentations
    augmented = transform(image=image, mask=mask)
    augmented_image = augmented["image"]
    augmented_mask = augmented["mask"]

    # Center crop for both image and mask
    height, width, _ = augmented_image.shape
    crop_height, crop_width = patch_size
    start_y = (height - crop_height) // 2
    start_x = (width - crop_width) // 2

    cropped_image = augmented_image[start_y:start_y + crop_height, start_x:start_x + crop_width]
    cropped_mask = augmented_mask[start_y:start_y + crop_height, start_x:start_x + crop_width]

    return cropped_image, cropped_mask

def save_augmented_images_and_masks(augmented_results, images_output_dir, masks_output_dir, base_filename):
    """
    Save the augmented images and masks to the specified directory.

    Args:
        augmented_results (list): List of tuples containing augmented image and mask pairs.
        output_dir (str): Directory to save the augmented images and masks.
        base_filename (str): Base filename for saving augmented images and masks (without extension).
    """
    # Ensure the output directory exists
    os.makedirs(images_output_dir, exist_ok=True)
    os.makedirs(masks_output_dir, exist_ok=True)

    for i, (augmented_image, augmented_mask) in enumerate(augmented_results):
        # Define filenames for augmented images and masks
        image_filename = os.path.join(images_output_dir, f"{base_filename}_augmented_{i + 1}.png")
        mask_filename = os.path.join(masks_output_dir, f"{base_filename}_augmented_{i + 1}.png")

        # Save the augmented image and mask to files
        cv2.imwrite(image_filename, augmented_image)
        cv2.imwrite(mask_filename, augmented_mask)

        print(f"Saved: {image_filename}, {mask_filename}")

def move_images_based_on_mask(mask_path, image_path, mask_filename, image_filename, normal_dir, anomaly_dir):
    """
    Move the image and mask to the appropriate folder based on mask content.

    Args:
        mask_path (str): Path to the mask file.
        image_path (str): Path to the image file.
        mask_filename (str): The mask filename.
        image_filename (str): The image filename.
        normal_dir (str): Directory to store normal images and masks.
        anomaly_dir (str): Directory to store anomaly images and masks.
    """
    # Load the mask (grayscale)
    mask = cv2.imread(mask_path, cv2.IMREAD_GRAYSCALE)

    # Check if there are any white pixels in the mask (indicating a defect)
    if cv2.countNonZero(mask) == 0 or np.unique(mask).size == 1:
        # No defects, move to normal folder
        normal_mask_path = os.path.join(normal_dir, 'masks', mask_filename)
        normal_image_path = os.path.join(normal_dir, 'images', image_filename)

        # Ensure the normal/masks and normal/images directories exist
        os.makedirs(os.path.dirname(normal_mask_path), exist_ok=True)
        os.makedirs(os.path.dirname(normal_image_path), exist_ok=True)
        
        # Move the files to the normal folder
        shutil.move(mask_path, normal_mask_path)
        shutil.move(image_path, normal_image_path)

        print(f"Moved {mask_filename} and {image_filename} to {normal_dir}")

    else:
        # Defects detected, move to anomaly folder
        anomaly_mask_path = os.path.join(anomaly_dir, 'masks', mask_filename)
        anomaly_image_path = os.path.join(anomaly_dir, 'images', image_filename)

        # Ensure the anomaly/masks and anomaly/images directories exist
        os.makedirs(os.path.dirname(anomaly_mask_path), exist_ok=True)
        os.makedirs(os.path.dirname(anomaly_image_path), exist_ok=True)
        
        # Move the files to the anomaly folder
        shutil.move(mask_path, anomaly_mask_path)
        shutil.move(image_path, anomaly_image_path)

        print(f"Moved {mask_filename} and {image_filename} to {anomaly_dir}")

def process_augmented_images_and_masks(augmented_masks_dir, augmented_images_dir, normal_dir, anomaly_dir):
    """
    Process all the augmented masks and images, moving them based on the content of the masks.

    Args:
        augmented_masks_dir (str): Directory containing augmented masks.
        augmented_images_dir (str): Directory containing augmented images.
        normal_dir (str): Directory to store normal images and masks.
        anomaly_dir (str): Directory to store anomaly images and masks.
    """
    # Ensure the normal and anomaly directories exist
    os.makedirs(normal_dir, exist_ok=True)
    os.makedirs(anomaly_dir, exist_ok=True)

    # Get the list of mask files
    mask_files = [f for f in os.listdir(augmented_masks_dir) if f.endswith('.png')]

    for mask_filename in mask_files:
        # Corresponding image filename (assuming same name, different extension)
        image_filename = mask_filename.replace("_mask", "")  # Remove '_mask' to get image name

        # Full paths to the mask and image
        mask_path = os.path.join(augmented_masks_dir, mask_filename)
        image_path = os.path.join(augmented_images_dir, image_filename)

        # Move the mask and corresponding image to the appropriate folder
        move_images_based_on_mask(mask_path, image_path, mask_filename, image_filename, normal_dir, anomaly_dir)

def main():
    # Set up argument parser
    parser = argparse.ArgumentParser(description='Train the UNet model on images and masks.')
    parser.add_argument('--source_dir', type=str, required=True, help=' Path to the directory containing the original images and masks.')
    parser.add_argument('--num_augmentations', type=int, required=True, help=' The number of augmented images for a single patch.')
    parser.add_argument('--patch_size', type=int, required=True, help=' The patch size of each augmented image.')

    args = parser.parse_args()
    source_dir = args.source_dir
    num_augmentations = args.num_augmentations
    patch_size = args.patch_size
    
    images_dir = os.path.join(source_dir, "images")
    masks_dir = os.path.join(source_dir, "masks")

    patches_output_dir = os.path.join(source_dir, "patches_output")
    masks_output_dir = os.path.join(source_dir, "masks_output")

    augmented_images_dir = os.path.join(source_dir, "augmented_images")
    augmented_masks_dir = os.path.join(source_dir, "augmented_masks")

    normal_dir = os.path.join(source_dir, "normal")
    anomaly_dir = os.path.join(source_dir, "anomaly")

    # Organizes files in the source directory into 'images' and 'masks' folders.
    organize_files(source_dir, images_dir, masks_dir)

    # Load paired data
    image_mask_pairs = load_image_mask_pairs(images_dir, masks_dir)

    # Patching
    for i, (image, mask, filename) in enumerate(image_mask_pairs):
        print(f"Pair {i}: Filename: {filename}, Image shape: {image.shape}, Mask shape: {mask.shape}")
        
        # Extract patches
        expanded_patch_size = patch_size * 2
        stride = 128
        patches = extract_patches(image, patch_size=expanded_patch_size, stride=stride)
        masks = extract_patches(mask, patch_size=expanded_patch_size, stride=stride)

        # Save patches
        save_patches(patches, patches_output_dir, prefix=f"{filename}_patch")
        save_patches(masks, masks_output_dir, prefix=f"{filename}_patch")

    # Load paired patch data
    image_mask_patches_pairs = load_image_mask_pairs(patches_output_dir, masks_output_dir)
    print(f"Loaded {len(image_mask_patches_pairs)} image-mask-patch pairs.")

    # Augmentation
    for i, (image, mask, filename) in enumerate(image_mask_patches_pairs):    
        # Generate augmented images and masks
        augmented_results = [
            augment_image_and_mask(image, mask, (patch_size, patch_size)) for _ in range(num_augmentations)
        ]
        
        # Define the output directory and base filename (you can customize these)
        base_filename = filename  # This will be used to generate filenames for the augmented images
        
        # Save the augmented images and masks
        save_augmented_images_and_masks(augmented_results, augmented_images_dir, augmented_masks_dir, base_filename)

    # Process and move the normal or anomaly folders
    process_augmented_images_and_masks(augmented_masks_dir, augmented_images_dir, normal_dir, anomaly_dir)

if __name__ == "__main__":
    main()
)";