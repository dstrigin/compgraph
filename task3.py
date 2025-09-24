import tkinter as tk
from tkinter import filedialog, messagebox
from tkinter import ttk
from PIL import Image, ImageTk
import numpy as np


# Функции преобразования RGB ↔ HSV
def rgb_to_hsv(r, g, b):
    r, g, b = r / 255.0, g / 255.0, b / 255.0
    max_val = max(r, g, b)
    min_val = min(r, g, b)
    delta = max_val - min_val

    if delta == 0:
        h = 0
    elif max_val == r:
        h = (60 * ((g - b) / delta) + 360) % 360
    elif max_val == g:
        h = (60 * ((b - r) / delta) + 120) % 360
    else:
        h = (60 * ((r - g) / delta) + 240) % 360

    s = 0 if max_val == 0 else (delta / max_val)
    v = max_val

    return h, s, v


def hsv_to_rgb(h, s, v):
    c = v * s
    x = c * (1 - abs(((h / 60) % 2) - 1))
    m = v - c

    if 0 <= h < 60:
        r, g, b = c, x, 0
    elif 60 <= h < 120:
        r, g, b = x, c, 0
    elif 120 <= h < 180:
        r, g, b = 0, c, x
    elif 180 <= h < 240:
        r, g, b = 0, x, c
    elif 240 <= h < 300:
        r, g, b = x, 0, c
    else:
        r, g, b = c, 0, x

    r, g, b = (r + m) * 255, (g + m) * 255, (b + m) * 255
    return int(r), int(g), int(b)


def process_image(image_path, hue_change, saturation_change, value_change):
    img = Image.open(image_path)
    img = img.convert("RGB")
    pixels = np.array(img)

    new_pixels = []
    for row in pixels:
        new_row = []
        for r, g, b in row:
            h, s, v = rgb_to_hsv(r, g, b)
            h = (h + hue_change) % 360
            s = np.clip(s + saturation_change, 0, 1)
            v = np.clip(v + value_change, 0, 1)
            new_r, new_g, new_b = hsv_to_rgb(h, s, v)
            new_row.append((new_r, new_g, new_b))
        new_pixels.append(new_row)

    new_image = Image.fromarray(np.array(new_pixels, dtype=np.uint8))
    new_image.save("modified_image.png")
    return new_image


class ImageHSVConverter:
    def __init__(self, root):
        self.root = root
        self.root.title("Преобразование HSV")
        self.hue = 0
        self.saturation = 0
        self.value = 0
        self.img_path = None
        self.img = None
        self.setup_ui()

    def setup_ui(self):
        self.label = ttk.Label(self.root, text="Выберите изображение и настройте параметры HSV",
                               font=('Arial', 14), justify=tk.CENTER)
        self.label.pack(pady=10)

        self.upload_button = ttk.Button(self.root, text="Загрузить изображение", command=self.load_image)
        self.upload_button.pack(pady=10)

        # Создаем фрейм для ползунков
        self.sliders_frame = ttk.Frame(self.root)
        self.sliders_frame.pack(fill='x', padx=20, pady=10)

        # Ползунок для оттенка
        self.hue_frame = ttk.Frame(self.sliders_frame)
        self.hue_frame.pack(fill='x', pady=5)

        self.hue_label = ttk.Label(self.hue_frame, text="Оттенок (H):", width=15, anchor='w')
        self.hue_label.pack(side='left')

        self.hue_value = ttk.Label(self.hue_frame, text="0%", width=6)
        self.hue_value.pack(side='right')

        self.hue_slider = ttk.Scale(self.hue_frame, from_=0, to_=100, orient='horizontal',
                                    command=self.update_hue)
        self.hue_slider.set(50)
        self.hue_slider.pack(side='right', fill='x', expand=True, padx=(0, 10))

        # Ползунок для насыщенности
        self.saturation_frame = ttk.Frame(self.sliders_frame)
        self.saturation_frame.pack(fill='x', pady=5)

        self.saturation_label = ttk.Label(self.saturation_frame, text="Насыщенность (S):", width=15, anchor='w')
        self.saturation_label.pack(side='left')

        self.saturation_value = ttk.Label(self.saturation_frame, text="0%", width=6)
        self.saturation_value.pack(side='right')

        self.saturation_slider = ttk.Scale(self.saturation_frame, from_=0, to_=100, orient='horizontal',
                                           command=self.update_saturation)
        self.saturation_slider.set(50)
        self.saturation_slider.pack(side='right', fill='x', expand=True, padx=(0, 10))

        # Ползунок для яркости
        self.value_frame = ttk.Frame(self.sliders_frame)
        self.value_frame.pack(fill='x', pady=5)

        self.value_label = ttk.Label(self.value_frame, text="Яркость (V):", width=15, anchor='w')
        self.value_label.pack(side='left')

        self.value_value = ttk.Label(self.value_frame, text="0%", width=6)
        self.value_value.pack(side='right')

        self.value_slider = ttk.Scale(self.value_frame, from_=0, to_=100, orient='horizontal',
                                      command=self.update_value)
        self.value_slider.set(50)
        self.value_slider.pack(side='right', fill='x', expand=True, padx=(0, 10))

        self.apply_button = ttk.Button(self.root, text="Применить изменения", command=self.apply_changes)
        self.apply_button.pack(pady=10)

        self.canvas = tk.Label(self.root)
        self.canvas.pack(pady=20)

    def load_image(self):
        self.img_path = filedialog.askopenfilename(filetypes=[("Image files", "*.jpg *.jpeg *.png *.bmp *.tiff")])
        if self.img_path:
            self.img = Image.open(self.img_path)
            self.display_image(self.img)

    def display_image(self, img):
        img = img.resize((400, 400), Image.Resampling.LANCZOS)
        img_tk = ImageTk.PhotoImage(img)
        self.canvas.config(image=img_tk)
        self.canvas.image = img_tk

    def update_hue(self, val):
        percentage = (float(val) - 50) * 2
        self.hue = percentage * 360 / 100
        self.hue_value.config(text=f"{int(percentage)}%")

    def update_saturation(self, val):
        percentage = (float(val) - 50) * 2
        self.saturation = percentage / 100
        self.saturation_value.config(text=f"{int(percentage)}%")

    def update_value(self, val):
        percentage = (float(val) - 50) * 2
        self.value = percentage / 100
        self.value_value.config(text=f"{int(percentage)}%")

    def apply_changes(self):
        if not self.img_path:
            messagebox.showerror("Ошибка", "Пожалуйста, загрузите изображение!")
            return

        modified_img = process_image(self.img_path, self.hue, self.saturation, self.value)
        self.display_image(modified_img)
        messagebox.showinfo("Готово", "Изображение сохранено как 'modified_image.png'!")
