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

