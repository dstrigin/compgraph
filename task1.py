import tkinter as tk
from tkinter import ttk, messagebox, filedialog
from PIL import Image, ImageTk
import numpy as np
import matplotlib.pyplot as plt
from io import BytesIO

def to_grayscale_pal(rgb_pixels):
    r, g, b = rgb_pixels[:, :, 0], rgb_pixels[:, :, 1], rgb_pixels[:, :, 2]
    return (0.299 * r + 0.587 * g + 0.114 * b).astype(np.uint8)

def to_grayscale_hdtv(rgb_pixels):
    r, g, b = rgb_pixels[:, :, 0], rgb_pixels[:, :, 1], rgb_pixels[:, :, 2]
    return (0.2126 * r + 0.7152 * g + 0.0722 * b).astype(np.uint8)

def get_difference_image(pal_img, hdtv_img):
    return np.abs(pal_img.astype(np.int16) - hdtv_img.astype(np.int16)).astype(np.uint8)

def create_histogram_image(image_data, title):
    plt.figure(figsize=(4, 3))
    plt.hist(image_data.flatten(), bins=256, range=(0, 256), color="black")
    plt.title(title)
    plt.xlabel("Интенсивность")
    plt.ylabel("Количество пикселей")
    plt.tight_layout()
    
    buf = BytesIO()
    plt.savefig(buf, format='png')
    plt.close()
    buf.seek(0)
    
    return Image.open(buf)

class ImageGrayConverter:
    def __init__(self, root):
        self.root = root
        self.root.title("Преобразование в оттенки серого")
        self.root.geometry("1000x750+200+100")
        self.root.minsize(800, 600)
        
        self.original_image = None
        
        self.setup_ui()
    
    def setup_ui(self):
        main_frame = ttk.Frame(self.root, padding=10)
        main_frame.pack(fill=tk.BOTH, expand=True)

        main_frame.rowconfigure(0, weight=1)
        main_frame.rowconfigure(1, weight=1)
        main_frame.rowconfigure(2, weight=0)
        main_frame.columnconfigure(0, weight=1)

        image_frame = ttk.LabelFrame(main_frame, text="Изображения")
        image_frame.grid(row=0, column=0, sticky="nsew", pady=5)
        
        hist_frame = ttk.LabelFrame(main_frame, text="Гистограммы")
        hist_frame.grid(row=1, column=0, sticky="nsew", pady=5)

        control_frame = ttk.Frame(main_frame)
        control_frame.grid(row=2, column=0, sticky="ew", pady=5)
        
        self.image_labels = {}
        titles = ["Оригинал", "PAL", "HDTV", "Разность"]
        for i, title in enumerate(titles):
            row, col = divmod(i, 2) 
            
            frame = ttk.Frame(image_frame)
            frame.grid(row=row, column=col, sticky="nsew", padx=5, pady=5)
            
            title_lbl = ttk.Label(frame, text=title, anchor="center")
            title_lbl.pack()
            
            img_lbl = ttk.Label(frame, background="white", anchor="center")
            img_lbl.pack(fill=tk.BOTH, expand=True)
            self.image_labels[title] = img_lbl

        image_frame.rowconfigure((0, 1), weight=1)
        image_frame.columnconfigure((0, 1), weight=1)

        self.hist_labels = {}
        hist_titles = ["PAL гистограмма", "HDTV гистограмма"]
        for i, title in enumerate(hist_titles):
            frame = ttk.Frame(hist_frame)
            frame.grid(row=0, column=i, sticky="nsew", padx=5, pady=5)
            hist_frame.columnconfigure(i, weight=1)
            
            img_lbl = ttk.Label(frame, background="lightgray", anchor="center")
            img_lbl.pack(fill=tk.BOTH, expand=True)
            self.hist_labels[title] = img_lbl
            
        self.load_btn = ttk.Button(control_frame, text="Загрузить изображение", command=self.load_image)
        self.load_btn.pack(side=tk.LEFT, padx=5)
        
        self.status_var = tk.StringVar(value="Готов к работе - загрузите изображение")
        status_bar = ttk.Label(self.root, textvariable=self.status_var, relief=tk.SUNKEN, anchor='w')
        status_bar.pack(side=tk.BOTTOM, fill=tk.X)

    def load_image(self):
        file_path = filedialog.askopenfilename(
            title="Выберите изображение",
            filetypes=[("Image files", "*.jpg *.jpeg *.png *.bmp *.tiff")]
        )
        if not file_path:
            return

        try:
            image = Image.open(file_path).convert("RGB")
            self.original_image = np.array(image)
            self.status_var.set(f"Изображение загружено: {file_path.split('/')[-1]}")
            self.process_and_display()
        except Exception as e:
            messagebox.showerror("Ошибка", f"Ошибка загрузки изображения: {e}")

    def process_and_display(self):
        if self.original_image is None:
            return

        pal_img_data = to_grayscale_pal(self.original_image)
        hdtv_img_data = to_grayscale_hdtv(self.original_image)
        diff_img_data = get_difference_image(pal_img_data, hdtv_img_data)

        pal_hist_img = create_histogram_image(pal_img_data, "PAL гистограмма")
        hdtv_hist_img = create_histogram_image(hdtv_img_data, "HDTV гистограмма")

        self.update_image_label(self.image_labels["Оригинал"], self.original_image)
        self.update_image_label(self.image_labels["PAL"], pal_img_data)
        self.update_image_label(self.image_labels["HDTV"], hdtv_img_data)
        self.update_image_label(self.image_labels["Разность"], diff_img_data)
        
        self.update_image_label(self.hist_labels["PAL гистограмма"], pal_hist_img, is_pil=True)
        self.update_image_label(self.hist_labels["HDTV гистограмма"], hdtv_hist_img, is_pil=True)

        
    def update_image_label(self, label, image_data, is_pil=False):
        if is_pil:
            pil_image = image_data.copy()
        else:
            pil_image = Image.fromarray(image_data)
        
        max_size = (350, 350) 
        pil_image.thumbnail(max_size, Image.Resampling.LANCZOS)
        
        tk_image = ImageTk.PhotoImage(pil_image)
        label.configure(image=tk_image)
        label.image = tk_image
