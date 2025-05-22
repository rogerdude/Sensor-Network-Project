import tkinter as tk
import math
import numpy as np
from scipy.ndimage import gaussian_filter
import matplotlib.pyplot as plt
import matplotlib.cm as cm
import matplotlib.colors as mcolors
import PIL.Image

# === Settings ===
scale = 0.333               # Metres per pixel
canvas_size = 600         # Heatmap resolution
sigma = 7                # Gaussian blur
vmin, vmax = 0, 100       # Colour mapping

# === Data ===
heatmap = np.zeros((canvas_size, canvas_size))
origin_lat = None
origin_lon = None

# === Matplotlib colormap ===
norm = mcolors.Normalize(vmin=vmin, vmax=vmax)
cmap = cm.get_cmap('hot')

# === Tkinter GUI ===
root = tk.Tk()
root.title("Live Gaussian GPS Heatmap")

# === Functions ===
def gps_to_relative_coords(lat0, lon0, lat1, lon1):
    R = 6371000  # Earth radius in metres
    dlat = math.radians(lat1 - lat0)
    dlon = math.radians(lon1 - lon0)
    avg_lat = math.radians((lat0 + lat1) / 2)
    dx = R * dlon * math.cos(avg_lat)
    dy = R * dlat
    return dx, -dy  # Flip y for screen

def add_gaussian_point(lat, lon, value):
    global origin_lat, origin_lon, heatmap

    if origin_lat is None:
        origin_lat, origin_lon = lat, lon
        print(f"[Origin set] lat={origin_lat}, lon={origin_lon}")

    dx, dy = gps_to_relative_coords(origin_lat, origin_lon, lat, lon)
    x = int(canvas_size // 2 + dx / scale)
    y = int(canvas_size // 2 + dy / scale)

    if 0 <= x < canvas_size and 0 <= y < canvas_size:
        heatmap[y, x] += value
        print(f"Added value={value} at pixel=({x},{y})")
    else:
        print(f"Point out of bounds: ({x},{y})")

def update_heatmap():
    blurred = gaussian_filter(heatmap, sigma=sigma)
    actual_min, actual_max = np.min(blurred), np.max(blurred)

    # Avoid 0 range for colour map (force scale from 0–vmax if empty)
    if actual_max == 0:
        actual_min, actual_max = 0, vmax
        print("Heatmap empty — showing base canvas with full scale.")

    ax.clear()
    im = ax.imshow(blurred, cmap='hot', origin='upper', vmin=actual_min, vmax=actual_max)
    ax.set_title("Live Gaussian Heatmap")
    ax.axis('off')

    if not hasattr(update_heatmap, "colorbar"):
        update_heatmap.colorbar = fig.colorbar(im, ax=ax, label="Sensor Value")
    else:
        update_heatmap.colorbar.update_normal(im)

    plt.pause(0.01)

def add_point():
    try:
        lat = float(lat_entry.get())
        lon = float(lon_entry.get())
        val = float(val_entry.get())
        add_gaussian_point(lat, lon, val)
        update_heatmap()

        lat_entry.delete(0, tk.END)
        lon_entry.delete(0, tk.END)
        val_entry.delete(0, tk.END)
    except ValueError:
        print("Invalid input. Please enter valid numbers.")

# === Input UI ===
tk.Label(root, text="Latitude").grid(row=1, column=0)
lat_entry = tk.Entry(root)
lat_entry.grid(row=1, column=1)

tk.Label(root, text="Longitude").grid(row=2, column=0)
lon_entry = tk.Entry(root)
lon_entry.grid(row=2, column=1)

tk.Label(root, text="Value").grid(row=3, column=0)
val_entry = tk.Entry(root)
val_entry.grid(row=3, column=1)

add_btn = tk.Button(root, text="Add Point", command=add_point)
add_btn.grid(row=1, column=2, rowspan=3, padx=10)

# === Matplotlib live figure setup ===
plt.ion()  # Interactive mode
fig, ax = plt.subplots(figsize=(8, 6))

def save_png_for_core2():
    fig.savefig("heatmap_raw.png", bbox_inches='tight')
    img = PIL.Image.open("heatmap_raw.png")
    img = img.resize((320, 240))  # M5Core2 screen resolution
    img.save("heatmap320.png")
    print("✅ Saved: heatmap320.png (ready for M5Core2)")

# Save after GUI closes
root.mainloop()
save_png_for_core2()
