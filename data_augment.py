import os
import argparse
import shutil
import cv2
import numpy as np
import albumentations as A

image_extensions = (".bmp", ".png", ".jpeg", ".jpg")
mask_extensions = ("_mask.bmp", "_mask.png", "_mask.jpeg", "_mask.jpg")

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
            if file_name.endswith(mask_extensions):
                # Move mask files to the 'masks' folder and rename them
                new_name = file_name.replace("_mask", "")
                shutil.move(file_path, os.path.join(masks_dir, new_name))
            elif file_name.endswith(image_extensions):
                # Move regular images to the 'images' folder
                shutil.move(file_path, os.path.join(images_dir, file_name))
    
    print(f"Files organized successfully into '{images_dir}' and '{masks_dir}'.")

def load_image_mask_pairs(images_dir, masks_dir, resize_to=None):
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
                # Resize image and mask if specified
                if resize_to is not None:
                    image = cv2.resize(image, resize_to, interpolation=cv2.INTER_LINEAR)
                    mask = cv2.resize(mask, resize_to, interpolation=cv2.INTER_NEAREST)

                # Store the pair along with the filename (without extension)
                filename = os.path.splitext(image_file)[0]  # Extract base filename without extension
                image_mask_pairs.append((image, mask, filename))
                print(f"Loaded image and mask: {image_path}, {mask_path}")
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
    # print(f"height: {height}, width: {width}")
    
    # Loop through rows
    y = 0
    while y < height:
        # print(f"y: {y}")
        # Break out of the loop for the last row patch
        if y + patch_size >= height:
            break
        # Loop through columns
        x = 0
        while x < width:
            # print(f"x: {x}")
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

def augment_image_and_mask(image, mask, patch_size=(512, 512), apply_shift_scale_rotate=True):    
    # Define the list of transformations
    transforms = []

    # Conditionally add ShiftScaleRotate
    if apply_shift_scale_rotate:
        transforms.append(
            A.ShiftScaleRotate(shift_limit=0.2, scale_limit=0.1, rotate_limit=5, border_mode=1)
        )

    # Add the rest of the transformations
    transforms.extend([
        A.HorizontalFlip(p=0.5),
        A.VerticalFlip(p=0.5),

        # Brightness and Contrast (applies only to the image)
        A.OneOf(
            [
                A.RandomBrightnessContrast(brightness_limit=(-0.15, 0.05), contrast_limit=0.15, p=0.5),
            ],
            p=0.5,  # Apply brightness/contrast with a probability
        ),
    ])

    # Define augmentation pipeline
    transform = A.Compose(
        transforms,
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

        print(f"Saved augmented: {image_filename}, {mask_filename}")

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

    if cv2.countNonZero(mask) == 0:
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
    
    # Create a mutually exclusive group to ensure only one of source_dir or patches_source_dir is provided
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--source_dir", type=str, default="", help="Path to the directory containing the original images and masks.")
    group.add_argument("--patches_source_dir", type=str, default="", help="Path to the directory containing the patched images and masks.")

    # parser.add_argument('--source_dir', type=str, default="", help=' Path to the directory containing the original images and masks.')
    # parser.add_argument('--patches_source_dir', type=str, default="", help=' Path to the directory containing the patched images and masks.')
    parser.add_argument('--num_augmentations', type=int, required=True, help=' The number of augmented images for a single patch.')
    parser.add_argument('--patch_size', type=int, required=True, help=' The patch size of each augmented image.')

    args = parser.parse_args()
    source_dir = args.source_dir
    patches_source_dir = args.patches_source_dir
    num_augmentations = args.num_augmentations
    patch_size = args.patch_size
    
    if os.path.exists(source_dir):
        images_dir = os.path.join(source_dir, "1_images")
        masks_dir = os.path.join(source_dir, "1_masks")

        patches_output_dir = os.path.join(source_dir, "2_patches_output")
        masks_output_dir = os.path.join(source_dir, "2_masks_output")

        augmented_images_dir = os.path.join(source_dir, "3_augmented_images")
        augmented_masks_dir = os.path.join(source_dir, "3_augmented_masks")

        normal_dir = os.path.join(source_dir, "4_normal")
        anomaly_dir = os.path.join(source_dir, "4_anomaly")

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
            augmented_results = []

            gray_mask = cv2.cvtColor(mask, cv2.COLOR_RGB2GRAY) if mask.ndim == 3 else mask
            if cv2.countNonZero(gray_mask) == 0:
                # No defects, don't augment
                augmented_results.append((image, mask))
            else:
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

    if os.path.exists(patches_source_dir):
        images_dir = os.path.join(patches_source_dir, "1_images")
        masks_dir = os.path.join(patches_source_dir, "1_masks")

        augmented_images_dir = os.path.join(patches_source_dir, "2_augmented_images")
        augmented_masks_dir = os.path.join(patches_source_dir, "2_augmented_masks")

        normal_dir = os.path.join(patches_source_dir, "3_normal")
        anomaly_dir = os.path.join(patches_source_dir, "3_anomaly")

        # Organizes files in the source directory into 'images' and 'masks' folders.
        organize_files(patches_source_dir, images_dir, masks_dir)

        # Load paired data and resize
        resize_to = (patch_size, patch_size)
        image_mask_pairs = load_image_mask_pairs(images_dir, masks_dir, resize_to)

        # Augmentation
        for i, (image, mask, filename) in enumerate(image_mask_pairs):
            augmented_results = []

            gray_mask = cv2.cvtColor(mask, cv2.COLOR_RGB2GRAY) if mask.ndim == 3 else mask
            if cv2.countNonZero(gray_mask) == 0:
                # No defects, don't augment
                augmented_results.append((image, mask))
            else:
                # Generate augmented images and masks
                augmented_results = [
                    augment_image_and_mask(image, mask, (patch_size, patch_size), False) for _ in range(num_augmentations)
                ]
            
            # Define the output directory and base filename (you can customize these)
            base_filename = filename  # This will be used to generate filenames for the augmented images
            
            # Save the augmented images and masks
            save_augmented_images_and_masks(augmented_results, augmented_images_dir, augmented_masks_dir, base_filename)

        # Process and move the normal or anomaly folders
        process_augmented_images_and_masks(augmented_masks_dir, augmented_images_dir, normal_dir, anomaly_dir)

if __name__ == "__main__":
    main()