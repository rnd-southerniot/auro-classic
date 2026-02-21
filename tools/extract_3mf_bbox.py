"""Extract basic dimensions from 3MF (3dmodel.model) files.

Usage:
  python3 tools/extract_3mf_bbox.py cad/extracted/frame/3D/3dmodel.model

Outputs:
- raw bbox extents (model axes)
- PCA-aligned extents (more robust when model is rotated)

Notes:
- 3MF unit is assumed millimeter if not specified.
"""

from __future__ import annotations

import sys
import xml.etree.ElementTree as ET
from pathlib import Path

import numpy as np

NS = {"m": "http://schemas.microsoft.com/3dmanufacturing/core/2015/02"}


def load_vertices(model_path: Path) -> tuple[str, np.ndarray]:
    root = ET.parse(model_path).getroot()
    unit = root.attrib.get("unit", "millimeter")
    verts = []
    for v in root.findall(".//m:vertex", NS):
        verts.append([float(v.get("x", "0")), float(v.get("y", "0")), float(v.get("z", "0"))])
    return unit, np.array(verts, dtype=float)


def bbox(v: np.ndarray) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    mins = v.min(axis=0)
    maxs = v.max(axis=0)
    return mins, maxs, (maxs - mins)


def pca_align(v: np.ndarray) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    c = v.mean(axis=0)
    x = v - c
    cov = np.cov(x.T)
    vals, vecs = np.linalg.eigh(cov)
    order = np.argsort(vals)[::-1]
    r = vecs[:, order]
    y = x @ r
    mins, maxs, size = bbox(y)
    return mins, maxs, size


def main() -> int:
    if len(sys.argv) != 2:
        print("Usage: python3 tools/extract_3mf_bbox.py <path-to-3dmodel.model>")
        return 2

    p = Path(sys.argv[1]).resolve()
    unit, v = load_vertices(p)
    print(f"file: {p}")
    print(f"unit: {unit}")
    print(f"verts: {len(v)}")

    mins, maxs, size = bbox(v)
    print("raw_bbox_min:", mins)
    print("raw_bbox_max:", maxs)
    print("raw_bbox_size:", size)

    pmins, pmaxs, psize = pca_align(v)
    print("pca_bbox_min:", pmins)
    print("pca_bbox_max:", pmaxs)
    print("pca_bbox_size:", psize)

    # axis ranking
    idx = np.argsort(psize)[::-1]
    print("pca_axis_order(largest->smallest):", idx)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
