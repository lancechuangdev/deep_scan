import os
import argparse
import numpy as np
import math
import tensorflow as tf
from tensorflow.keras.utils import image_dataset_from_directory
from tensorflow.keras.models import load_model
from PIL import Image

def load_images_and_masks(image_dir, mask_dir, target_size=(256, 256), batch_size=8):
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
    loaded_model = load_model(model_path, custom_objects=custom_objects)

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
                patch_with_border = np.pad(images[patch_idx], ((border_thickness, border_thickness),
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

        # Normalize true and predicted masks to [0, 255] range for saving
        #stitched_true_mask = (stitched_true_mask * 255).astype(np.uint8)
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