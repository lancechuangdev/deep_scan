import os
import argparse
import tensorflow as tf
from tensorflow.keras.utils import image_dataset_from_directory
from tensorflow.keras.layers import Input, Conv2D, MaxPooling2D, UpSampling2D, concatenate
from tensorflow.keras.models import Model
from tensorflow.keras.optimizers import Adam
from tensorflow.keras.callbacks import ModelCheckpoint
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

def load_images_and_masks(image_dir, mask_dir, target_size=(256, 256), batch_size=8, normalize=True, augment=True):
    # Load images
    image_dataset = image_dataset_from_directory(
        image_dir,
        labels=None,
        image_size=target_size,
        batch_size=batch_size,
        shuffle=False
    )

    # Load masks
    mask_dataset = image_dataset_from_directory(
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

    # Normalize the images and masks to [0, 1]
    # Masks have a pixel value of 0 for the background and 255 for the foreground, normalize the values by dividing by 255.0
    if normalize:
        dataset = dataset.map(lambda img, mask: (tf.image.convert_image_dtype(img, tf.float32),
                                                 tf.image.convert_image_dtype(mask, tf.float32) / 255.0))

    return dataset

# U-Net model definition
def unet_model(input_size=(256, 256, 3)):
    # Input layerE
    inputs = Input(input_size)

    # Encoding path
    c1 = Conv2D(16, (3, 3), activation='relu', padding='same')(inputs)
    c1 = Conv2D(16, (3, 3), activation='relu', padding='same')(c1)
    p1 = MaxPooling2D((2, 2))(c1)

    c2 = Conv2D(32, (3, 3), activation='relu', padding='same')(p1)
    c2 = Conv2D(32, (3, 3), activation='relu', padding='same')(c2)
    p2 = MaxPooling2D((2, 2))(c2)

    c3 = Conv2D(64, (3, 3), activation='relu', padding='same')(p2)
    c3 = Conv2D(64, (3, 3), activation='relu', padding='same')(c3)
    p3 = MaxPooling2D(pool_size=(2, 2))(c3)

    c4 = Conv2D(128, (3, 3), activation='relu', padding='same')(p3)
    c4 = Conv2D(128, (3, 3), activation='relu', padding='same')(c4)
    p4 = MaxPooling2D(pool_size=(2, 2))(c4)

    # Bottleneck
    c5 = Conv2D(256, (3, 3), activation='relu', padding='same')(p4)
    c5 = Conv2D(256, (3, 3), activation='relu', padding='same')(c5)

    # Decoding path
    u6 = concatenate([UpSampling2D((2, 2))(c5), c4])
    c6 = Conv2D(128, (3, 3), activation='relu', padding='same')(u6)
    c6 = Conv2D(128, (3, 3), activation='relu', padding='same')(c6)

    u7 = concatenate([UpSampling2D((2, 2))(c6), c3])
    c7 = Conv2D(64, (3, 3), activation='relu', padding='same')(u7)
    c7 = Conv2D(64, (3, 3), activation='relu', padding='same')(c7)

    u8 = concatenate([UpSampling2D((2, 2))(c7), c2])
    c8 = Conv2D(32, (3, 3), activation='relu', padding='same')(u8)
    c8 = Conv2D(32, (3, 3), activation='relu', padding='same')(c8)

    u9 = concatenate([UpSampling2D((2, 2))(c8), c1])
    c9 = Conv2D(16, (3, 3), activation='relu', padding='same')(u9)
    c9 = Conv2D(16, (3, 3), activation='relu', padding='same')(c9)

    # Output layer
    outputs = Conv2D(1, (1, 1), activation='sigmoid')(c9)

    model = Model(inputs=[inputs], outputs=[outputs])
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
    model = unet_model(input_size=(patch_size, patch_size, 3))

    # Train the model
    model.compile(optimizer=Adam(learning_rate=1e-4), loss='binary_crossentropy', metrics=[iou])
    checkpoint_callback = ModelCheckpoint(model_path, save_best_only=True)
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