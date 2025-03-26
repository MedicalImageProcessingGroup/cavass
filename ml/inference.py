#!/usr/bin/python3

"""
usage example:
python MedSAM_Inference.py -i assets/img_demo.png -o ./ --box "[95,255,190,350]"

./gjg.py --box [232,107,257,131]  \
         --data_path /tmp/cavass-seg2dintdl-GEv1wu.pgm  \
         --device cpu  \
         --seg_path /tmp  \
         --checkpoint /home/jorge/mipg/MedSAM/work_dir/MedSAM/medsam_vit_b.pth
"""

import time

verbose = False

print( 'start (time reported in sec)' )
overall = start = time.time()

# %% load environment
import numpy as np
import matplotlib.pyplot as plt
import os

join = os.path.join
import torch
from segment_anything import sam_model_registry
from skimage import io, transform
import torch.nn.functional as F
import argparse

if torch.cuda.is_available():    print( 'cuda is available' )
else:                            print( 'cuda is not available' )


# visualization functions
# source: https://github.com/facebookresearch/segment-anything/blob/main/notebooks/predictor_example.ipynb
# change color to avoid red and green
def show_mask(mask, ax, random_color=False):
    if random_color:
        color = np.concatenate([np.random.random(3), np.array([0.6])], axis=0)
    else:
        color = np.array([251 / 255, 252 / 255, 30 / 255, 0.6])
    h, w = mask.shape[-2:]
    mask_image = mask.reshape(h, w, 1) * color.reshape(1, 1, -1)
    ax.imshow(mask_image)


def show_box(box, ax):
    x0, y0 = box[0], box[1]
    w, h = box[2] - box[0], box[3] - box[1]
    ax.add_patch(
        plt.Rectangle((x0, y0), w, h, edgecolor="blue", facecolor=(0, 0, 0, 0), lw=2)
    )


@torch.no_grad()
def medsam_inference(medsam_model, img_embed, box_1024, H, W):
    box_torch = torch.as_tensor(box_1024, dtype=torch.float, device=img_embed.device)
    if len(box_torch.shape) == 2:
        box_torch = box_torch[:, None, :]  # (B, 1, 4)

    sparse_embeddings, dense_embeddings = medsam_model.prompt_encoder(
        points=None,
        boxes=box_torch,
        masks=None,
    )
    low_res_logits, _ = medsam_model.mask_decoder(
        image_embeddings=img_embed,  # (B, 256, 64, 64)
        image_pe=medsam_model.prompt_encoder.get_dense_pe(),  # (1, 256, 64, 64)
        sparse_prompt_embeddings=sparse_embeddings,  # (B, 2, 256)
        dense_prompt_embeddings=dense_embeddings,  # (B, 256, 64, 64)
        multimask_output=False,
    )

    low_res_pred = torch.sigmoid(low_res_logits)  # (1, 1, 256, 256)

    low_res_pred = F.interpolate(
        low_res_pred,
        size=(H, W),
        mode="bilinear",
        align_corners=False,
    )  # (1, 1, gt.shape)
    low_res_pred = low_res_pred.squeeze().cpu().numpy()  # (256, 256)
    medsam_seg = (low_res_pred > 0.5).astype(np.uint8)
    return medsam_seg


# %% load model and image
parser = argparse.ArgumentParser(
    description="run inference on testing set based on MedSAM"
)
parser.add_argument(
    "-i",
    "--data_path",
    type=str,
    default="assets/img_demo.png",
    help="path to the data folder",
)
parser.add_argument(
    "-o",
    "--seg_path",
    type=str,
    default="assets/",
    help="path to the segmentation folder",
)
parser.add_argument(
    "--box",
    type=str,
    default='[95, 255, 190, 350]',
    help="bounding box of the segmentation target",
)
parser.add_argument("--device", type=str, default="cuda:0", help="device")
parser.add_argument(
    "-chk",
    "--checkpoint",
    type=str,
    default="work_dir/MedSAM/medsam_vit_b.pth",
    help="path to the trained model",
)
args = parser.parse_args()

device = args.device
print( 'start up e.t. = %.2f' % (time.time()-start) )

start = time.time()  # ================================================
medsam_model = sam_model_registry["vit_b"](checkpoint=args.checkpoint)
medsam_model = medsam_model.to(device)
medsam_model.eval()
print( 'load model file e.t. = %.2f' % (time.time()-start) )
if verbose:    print( medsam_model )

start = time.time()  # ================================================
img_np = io.imread(args.data_path)
print( 'load image from file e.t. = %.2f' % (time.time()-start) )

#gjg: normalize (otherwise, 16-bit data > 255 will be clamped to 255)
start = time.time()  # ================================================
min = np.min( img_np );  max = np.max( img_np );  print( 'min =', min, 'max =', max )
img_np = img_np / max
# min = np.min( img_np );  max = np.max( img_np );  print( 'min =', min, 'max =', max )
print( 'normalize image e.t. = %.2f' % (time.time()-start) )

start = time.time()  # ================================================
if len(img_np.shape) == 2:
    img_3c = np.repeat(img_np[:, :, None], 3, axis=-1)
else:
    img_3c = img_np
H, W, _ = img_3c.shape
print( 'gray2color conversion e.t. = %.2f' % (time.time()-start) )

# %% image preprocessing
start = time.time()  # ======================================================
img_1024 = transform.resize(
    img_3c, (1024, 1024), order=3, preserve_range=True, anti_aliasing=True
)  # .astype(np.uint8)
print( 'resize/interpolate e.t. = %.2f' % (time.time()-start) )

start = time.time()  # ======================================================
img_1024 = (img_1024 - img_1024.min()) / np.clip(
    img_1024.max() - img_1024.min(), a_min=1e-8, a_max=None
)  # normalize to [0, 1], (H, W, 3)
print( 'normalize e.t. = %.2f' % (time.time()-start) )

start = time.time()  # ======================================================
# convert the shape to (3, H, W)
img_1024_tensor = (
    torch.tensor(img_1024).float().permute(2, 0, 1).unsqueeze(0).to(device)
)
print( 'create input tensor e.t. = %.2f' % (time.time()-start) )

start = time.time()  # ======================================================
box_np = np.array([[int(x) for x in args.box[1:-1].split(',')]])
# transfer box_np t0 1024x1024 scale
box_1024 = box_np / np.array([W, H, W, H]) * 1024
print( 'resize box e.t. = %.2f' % (time.time()-start) )

start = time.time()  # ======================================================
with torch.no_grad():
    image_embedding = medsam_model.image_encoder(img_1024_tensor)  # (1, 256, 64, 64)
print( 'image encoder e.t. = %.2f' % (time.time()-start) )

start = time.time()  # ======================================================
medsam_seg = medsam_inference(medsam_model, image_embedding, box_1024, H, W)
print( 'inference e.t. = %.2f' % (time.time()-start) )

start = time.time()  # ======================================================
io.imsave(
    join(args.seg_path, "seg_" + os.path.basename(args.data_path)),
    medsam_seg,
    check_contrast=False,
)
print( 'save result to file e.t. = %.2f' % (time.time()-start) )

print( 'finished: overall e.t. = %.2f' % (time.time()-overall) )

if verbose:
    # %% visualize results
    fig, ax = plt.subplots(1, 2, figsize=(10, 5))
    ax[0].imshow(img_3c)
    show_box(box_np[0], ax[0])
    ax[0].set_title("Input Image and Bounding Box")
    ax[1].imshow(img_3c)
    show_mask(medsam_seg, ax[1])
    show_box(box_np[0], ax[1])
    ax[1].set_title("MedSAM Segmentation")
    plt.show()

