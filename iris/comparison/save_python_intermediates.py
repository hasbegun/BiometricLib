#!/usr/bin/env python3
"""
Save Python open-iris intermediate pipeline results for comparison with C++.
Processes first N images and saves geometry, normalized image, segmentation, etc.
"""

import os
import sys

import cv2
import numpy as np


def main():
    dataset_dir = sys.argv[1] if len(sys.argv) > 1 else "/Users/innox/projects/iris2/data/Iris/CASIA1"
    output_dir = sys.argv[2] if len(sys.argv) > 2 else "/Users/innox/projects/iris2/iriscpp/biometric/iris/comparison/python_results/templates"
    max_images = int(sys.argv[3]) if len(sys.argv) > 3 else 10

    os.makedirs(output_dir, exist_ok=True)

    import iris
    from pathlib import Path

    pipeline = iris.IRISPipeline()

    # Discover images (same as python_baseline.py)
    images = []
    for subject_dir in sorted(Path(dataset_dir).iterdir()):
        if not subject_dir.is_dir():
            continue
        for img_file in sorted(subject_dir.glob("*.jpg")):
            parts = img_file.stem.split("_")
            if len(parts) != 3:
                continue
            images.append({"path": str(img_file), "filename": img_file.stem})

    count = 0
    for img_info in images:
        if count >= max_images:
            break

        filename = img_info["filename"]
        print(f"Processing {filename}...")

        img = cv2.imread(img_info["path"], cv2.IMREAD_GRAYSCALE)
        if img is None:
            print(f"  SKIP: cannot read")
            continue

        ir_image = iris.IRImage(img_data=img, image_id=filename, eye_side="left")

        try:
            output = pipeline(ir_image)
        except Exception as e:
            print(f"  FAIL: {e}")
            continue

        if output["error"] is not None:
            print(f"  FAIL: {output['error']}")
            continue

        ct = pipeline.call_trace

        # 1. Save segmentation map
        seg = ct.get("segmentation")
        if seg is not None:
            # SegmentationMap has predictions attribute
            for attr_name in dir(seg):
                if attr_name.startswith('_'):
                    continue
                val = getattr(seg, attr_name)
                if isinstance(val, np.ndarray):
                    np.save(os.path.join(output_dir, f"{filename}_seg_{attr_name}.npy"), val)
                    print(f"  seg.{attr_name}: shape={val.shape}, dtype={val.dtype}")

        # 2. Save geometry contours
        geom = ct.get("geometry_estimation")
        if geom is not None:
            # GeometryPolygons has iris, pupil, eyeball
            for attr_name in ["iris_array", "pupil_array", "eyeball_array"]:
                if hasattr(geom, attr_name):
                    val = getattr(geom, attr_name)
                    if isinstance(val, np.ndarray):
                        np.save(os.path.join(output_dir, f"{filename}_geom_{attr_name}.npy"), val)
                        print(f"  geom.{attr_name}: shape={val.shape}, dtype={val.dtype}")

            # Try direct attributes
            for attr_name in ["iris", "pupil", "eyeball"]:
                if hasattr(geom, attr_name):
                    val = getattr(geom, attr_name)
                    if isinstance(val, np.ndarray):
                        np.save(os.path.join(output_dir, f"{filename}_geom_{attr_name}.npy"), val)
                        print(f"  geom.{attr_name}: shape={val.shape}, dtype={val.dtype}")
                    elif hasattr(val, '__len__'):
                        arr = np.array(val)
                        np.save(os.path.join(output_dir, f"{filename}_geom_{attr_name}.npy"), arr)
                        print(f"  geom.{attr_name}: shape={arr.shape}, dtype={arr.dtype}")

            # Dump all attributes
            print(f"  geom type: {type(geom).__name__}")
            print(f"  geom attrs: {[a for a in dir(geom) if not a.startswith('_')]}")

        # 3. Save eye orientation
        orient = ct.get("eye_orientation")
        if orient is not None:
            print(f"  orientation type: {type(orient).__name__}")
            print(f"  orientation attrs: {[a for a in dir(orient) if not a.startswith('_')]}")
            if hasattr(orient, 'angle'):
                print(f"  orientation.angle = {orient.angle}")
                with open(os.path.join(output_dir, f"{filename}_py_orientation.txt"), "w") as f:
                    f.write(f"angle_radians={orient.angle}\n")

        # 4. Save eye centers
        centers = ct.get("eye_center_estimation")
        if centers is not None:
            print(f"  centers type: {type(centers).__name__}")
            print(f"  centers attrs: {[a for a in dir(centers) if not a.startswith('_')]}")
            with open(os.path.join(output_dir, f"{filename}_py_centers.txt"), "w") as f:
                for attr_name in dir(centers):
                    if attr_name.startswith('_'):
                        continue
                    val = getattr(centers, attr_name)
                    if not callable(val):
                        f.write(f"{attr_name}={val}\n")
                        print(f"  centers.{attr_name} = {val}")

        # 5. Save normalized iris
        norm = ct.get("normalization")
        if norm is not None:
            if hasattr(norm, 'normalized_image'):
                ni = norm.normalized_image
                np.save(os.path.join(output_dir, f"{filename}_py_norm_image.npy"), ni)
                print(f"  norm_image: shape={ni.shape}, dtype={ni.dtype}, "
                      f"range=[{ni.min():.2f}, {ni.max():.2f}], mean={ni.mean():.2f}")
            if hasattr(norm, 'normalized_mask'):
                nm = norm.normalized_mask
                np.save(os.path.join(output_dir, f"{filename}_py_norm_mask.npy"), nm)
                print(f"  norm_mask: shape={nm.shape}, dtype={nm.dtype}")

        # 6. Save iris response (filter bank output)
        iris_resp = ct.get("filter_bank")
        if iris_resp is not None:
            print(f"  filter_bank type: {type(iris_resp).__name__}")
            print(f"  filter_bank attrs: {[a for a in dir(iris_resp) if not a.startswith('_')]}")
            if hasattr(iris_resp, 'iris_responses'):
                for j, resp in enumerate(iris_resp.iris_responses):
                    np.save(os.path.join(output_dir, f"{filename}_py_response_{j}.npy"), resp)
                    print(f"  response[{j}]: shape={resp.shape}, dtype={resp.dtype}")

        # 7. Save iris codes
        template = output["iris_template"]
        codes = np.stack(template.iris_codes)
        masks = np.stack(template.mask_codes)
        np.save(os.path.join(output_dir, f"{filename}_py_iris_codes.npy"), codes)
        np.save(os.path.join(output_dir, f"{filename}_py_mask_codes.npy"), masks)
        print(f"  iris_codes: shape={codes.shape}, dtype={codes.dtype}")

        count += 1
        print()

    print(f"\nDone. Saved intermediate results for {count} images to {output_dir}")


if __name__ == "__main__":
    main()
