import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from PIL import Image, ImageTk
import cv2
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg

class ImageChannelViewer:
    def __init__(self, root):
        self.root = root
        self.root.title("Задание 2: Анализ цветовых каналов")
        
        self.original_image = None
        self.current_image = None
        
        self.setup_ui()
    
    def setup_ui(self):
        main_frame = ttk.Frame(self.root)
        main_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        control_frame = ttk.Frame(main_frame)
        control_frame.pack(fill=tk.X, pady=10)
        
        self.load_btn = ttk.Button(control_frame, text="Загрузить изображение", 
            command=self.load_image)
        self.load_btn.pack(side=tk.LEFT, padx=5)
        
        self.red_btn = ttk.Button(control_frame, text="Красный канал (R)", 
            command=self.show_red_channel, state=tk.DISABLED)
        self.red_btn.pack(side=tk.LEFT, padx=5)
        
        self.green_btn = ttk.Button(control_frame, text="Зеленый канал (G)", 
            command=self.show_green_channel, state=tk.DISABLED)
        self.green_btn.pack(side=tk.LEFT, padx=5)
        
        self.blue_btn = ttk.Button(control_frame, text="Синий канал (B)", 
            command=self.show_blue_channel, state=tk.DISABLED)
        self.blue_btn.pack(side=tk.LEFT, padx=5)
        
        self.hist_btn = ttk.Button(control_frame, text="Построить гистограммы", 
            command=self.show_histograms, state=tk.DISABLED)
        self.hist_btn.pack(side=tk.LEFT, padx=5)
        
        self.original_btn = ttk.Button(control_frame, text="Оригинал", 
            command=self.show_original, state=tk.DISABLED)
        self.original_btn.pack(side=tk.LEFT, padx=5)
        
        image_frame = ttk.Frame(main_frame)
        image_frame.pack(fill=tk.BOTH, expand=True)
        
        self.image_label = ttk.Label(image_frame, background='white')
        self.image_label.pack(expand=True)
        
        self.status_var = tk.StringVar()
        self.status_var.set("Готов к работе - загрузите изображение")
        status_bar = ttk.Label(main_frame, textvariable=self.status_var, relief=tk.SUNKEN)
        status_bar.pack(fill=tk.X, pady=5)
    
    def load_image(self):
        file_path = filedialog.askopenfilename(
            title="Выберите изображение",
            filetypes=[("Image files", "*.jpg *.jpeg *.png *.bmp *.tiff")]
        )
        
        if file_path:
            try:
                self.original_image = cv2.imread(file_path)
                if self.original_image is None:
                    raise ValueError("Не удалось загрузить изображение")
                
                self.original_image = cv2.cvtColor(self.original_image, cv2.COLOR_BGR2RGB)
                self.current_image = self.original_image.copy()
                
                self.show_image(self.current_image)
                self.enable_buttons()
                self.status_var.set(f"Изображение загружено: {file_path}")
                
            except Exception as e:
                messagebox.showerror("Ошибка", f"Ошибка загрузки изображения: {str(e)}")
    
    def enable_buttons(self):
        self.red_btn.config(state=tk.NORMAL)
        self.green_btn.config(state=tk.NORMAL)
        self.blue_btn.config(state=tk.NORMAL)
        self.hist_btn.config(state=tk.NORMAL)
        self.original_btn.config(state=tk.NORMAL)
    
    def show_image(self, image):
        h, w = image.shape[:2]
        max_size = 600
        scale = min(max_size/w, max_size/h)
        
        new_w, new_h = int(w*scale), int(h*scale)
        resized_image = cv2.resize(image, (new_w, new_h))
        
        pil_image = Image.fromarray(resized_image)
        tk_image = ImageTk.PhotoImage(pil_image)
        
        self.image_label.configure(image=tk_image)
        self.image_label.image = tk_image
    
    def show_red_channel(self):
        if self.original_image is not None:
            red_channel = self.original_image.copy()
            red_channel[:, :, 1] = 0  # G
            red_channel[:, :, 2] = 0  # B
            self.current_image = red_channel
            self.show_image(red_channel)
            self.status_var.set("Отображен красный канал")
    
    def show_green_channel(self):
        if self.original_image is not None:
            green_channel = self.original_image.copy()
            green_channel[:, :, 0] = 0  # R
            green_channel[:, :, 2] = 0  # B
            self.current_image = green_channel
            self.show_image(green_channel)
            self.status_var.set("Отображен зеленый канал")
    
    def show_blue_channel(self):
        if self.original_image is not None:
            blue_channel = self.original_image.copy()
            blue_channel[:, :, 0] = 0  # R
            blue_channel[:, :, 1] = 0  # G
            self.current_image = blue_channel
            self.show_image(blue_channel)
            self.status_var.set("Отображен синий канал")
    
    def show_original(self):
        if self.original_image is not None:
            self.current_image = self.original_image.copy()
            self.show_image(self.original_image)
            self.status_var.set("Отображено оригинальное изображение")
    
    def show_histograms(self):
        if self.original_image is None:
            return
        
        hist_window = tk.Toplevel(self.root)
        hist_window.title("Гистограммы цветовых каналов")
        hist_window.geometry("900x600")
        
        channels = cv2.split(self.original_image)
        colors = ('red', 'green', 'blue')
        channel_names = ('Красный', 'Зеленый', 'Синий')
        
        fig, axes = plt.subplots(2, 2, figsize=(10, 8))
        fig.suptitle('Гистограммы цветовых каналов', fontsize=16)
        
        axes[0, 0].hist(channels[0].ravel(), 256, [0, 256], color='red', alpha=0.7)
        axes[0, 0].set_title('Красный канал')
        axes[0, 0].set_xlim([0, 256])
        
        axes[0, 1].hist(channels[1].ravel(), 256, [0, 256], color='green', alpha=0.7)
        axes[0, 1].set_title('Зеленый канал')
        axes[0, 1].set_xlim([0, 256])
        
        axes[1, 0].hist(channels[2].ravel(), 256, [0, 256], color='blue', alpha=0.7)
        axes[1, 0].set_title('Синий канал')
        axes[1, 0].set_xlim([0, 256])
        
        for i, color in enumerate(colors):
            axes[1, 1].hist(channels[i].ravel(), 256, [0, 256], color=color, alpha=0.5, label=channel_names[i])
        axes[1, 1].set_title('Все каналы')
        axes[1, 1].legend()
        axes[1, 1].set_xlim([0, 256])
        
        plt.tight_layout()
        
        canvas = FigureCanvasTkAgg(fig, master=hist_window)
        canvas.draw()
        canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)
        
        save_btn = ttk.Button(hist_window, text="Сохранить гистограммы", 
                             command=lambda: self.save_histograms(fig))
        save_btn.pack(pady=10)
        
        self.status_var.set("Построены гистограммы цветовых каналов")
    
    def save_histograms(self, fig):
        file_path = filedialog.asksaveasfilename(
            defaultextension=".png",
            filetypes=[("PNG files", "*.png"), ("All files", "*.*")]
        )
        if file_path:
            fig.savefig(file_path, dpi=300, bbox_inches='tight')
            messagebox.showinfo("Успех", f"Гистограммы сохранены в: {file_path}")