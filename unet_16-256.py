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