import tkinter as tk
from tkinter import ttk, filedialog, messagebox, colorchooser
from PIL import Image, ImageTk, ImageDraw
import numpy as np

class ColorFiller:
    def __init__(self, root):
        self.root = root
        self.root.title("Преобразование в оттенки серого")
        self.root.geometry("1000x750+200+100")
        self.root.minsize(800, 600)
        
        self.drawing = False
        self.last_x, self.last_y = None, None
        self.fill_color = "blue"
        self.border_color = "red"
        self.pattern_image = None
        
        self.create_widgets()
        
        self.init_canvas_image()
    
    def create_widgets(self):
        main_frame = ttk.Frame(self.root)
        main_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        left_frame = ttk.Frame(main_frame)
        left_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        
        ttk.Label(left_frame, text="Область для рисования", font=('Arial', 12)).pack(pady=5)
        
        self.canvas = tk.Canvas(left_frame, bg="white", width=600, height=500)
        self.canvas.pack(pady=10)
        
        self.canvas.bind("<Button-1>", self.start_drawing)
        self.canvas.bind("<B1-Motion>", self.draw)
        self.canvas.bind("<ButtonRelease-1>", self.stop_drawing)
        self.canvas.bind("<Button-3>", self.set_fill_point)
        
        right_frame = ttk.Frame(main_frame, width=300)
        right_frame.pack(side=tk.RIGHT, fill=tk.Y, padx=(20, 0))
        right_frame.pack_propagate(False)
        
        ttk.Label(right_frame, text="Управление", font=('Arial', 14)).pack(pady=10)
        
        fill_frame = ttk.LabelFrame(right_frame, text="Заливка", padding=10)
        fill_frame.pack(fill=tk.X, pady=10)
        
        ttk.Button(fill_frame, text="Залить цветом", 
                  command=self.fill_color_dialog).pack(fill=tk.X, pady=5)
        
        ttk.Button(fill_frame, text="Залить изображением", 
                  command=self.fill_pattern_dialog).pack(fill=tk.X, pady=5)
        
        border_frame = ttk.LabelFrame(right_frame, text="Границы", padding=10)
        border_frame.pack(fill=tk.X, pady=10)
        
        ttk.Button(border_frame, text="Выделить границу области", 
                  command=self.detect_border).pack(fill=tk.X, pady=5)
        
        ttk.Button(border_frame, text="Показать границу", 
                  command=self.show_border).pack(fill=tk.X, pady=5)
        
        tools_frame = ttk.LabelFrame(right_frame, text="Инструменты", padding=10)
        tools_frame.pack(fill=tk.X, pady=10)
        
        ttk.Button(tools_frame, text="Очистить холст", 
                  command=self.clear_canvas).pack(fill=tk.X, pady=5)
        
        ttk.Button(tools_frame, text="Загрузить изображение", 
                  command=self.load_image).pack(fill=tk.X, pady=5)
        
        info_frame = ttk.LabelFrame(right_frame, text="Информация", padding=10)
        info_frame.pack(fill=tk.X, pady=10)
        
        self.info_label = ttk.Label(info_frame, text="Рисуйте мышью на холсте\nПравый клик - точка заливки", 
                                   wraplength=250, justify=tk.CENTER)
        self.info_label.pack()
        
        self.status_label = ttk.Label(right_frame, text="Готов к работе", relief=tk.SUNKEN)
        self.status_label.pack(side=tk.BOTTOM, fill=tk.X, pady=5)
    
    def init_canvas_image(self):
        self.canvas_width = 600
        self.canvas_height = 500
        self.image = Image.new("RGB", (self.canvas_width, self.canvas_height), "white")
        self.draw = ImageDraw.Draw(self.image)
        self.photo = ImageTk.PhotoImage(self.image)
        self.canvas_image = self.canvas.create_image(0, 0, anchor=tk.NW, image=self.photo)
    
    def start_drawing(self, event):
        self.drawing = True
        self.last_x, self.last_y = event.x, event.y
    
    def draw(self, event):
        if self.drawing and self.last_x and self.last_y:
            self.draw.line([self.last_x, self.last_y, event.x, event.y], 
                          fill="black", width=2)
            
            self.photo = ImageTk.PhotoImage(self.image)
            self.canvas.itemconfig(self.canvas_image, image=self.photo)
            
            self.last_x, self.last_y = event.x, event.y
    
    def stop_drawing(self, event):
        self.drawing = False
        self.last_x, self.last_y = None, None
    
    def set_fill_point(self, event):
        self.fill_point = (event.x, event.y)
        self.canvas.create_oval(event.x-3, event.y-3, event.x+3, event.y+3, 
                               fill="red", outline="red", tags="fill_point")
        self.status_label.config(text=f"Точка заливки: ({event.x}, {event.y})")
    
    def fill_color_dialog(self):
        color_tuple = colorchooser.askcolor(title="Выберите цвет заливки")
        if color_tuple and color_tuple[1]:
            hex_color = color_tuple[1]
            rgb_color = color_tuple[0]
            
            self.fill_color = hex_color
            self.status_label.config(text=f"Цвет заливки: {hex_color}")
            
            if hasattr(self, 'fill_point'):
                x, y = self.fill_point
                target_color = self.image.getpixel((x, y))
                
                fill_rgb = (int(rgb_color[0]), int(rgb_color[1]), int(rgb_color[2]))
                
                self._flood_fill_recursive(x, y, fill_rgb, target_color)
                
                self.photo = ImageTk.PhotoImage(self.image)
                self.canvas.itemconfig(self.canvas_image, image=self.photo)
                
            else:
                messagebox.showerror("Ошибка", "Сначала поставьте точку заливки с помощью ПКМ")
    
    def _flood_fill_recursive(self, x, y, color, target_color):
        if x < 0 or x >= self.canvas_width or y < 0 or y >= self.canvas_height:
            return
        current_color = self.image.getpixel((x, y))
        if current_color != target_color:
            return
        
        x_left = x
        while x_left > 0 and self.image.getpixel((x_left - 1, y)) == target_color:
            x_left -= 1

        x_right = x
        while x_right < self.canvas_width - 1 and self.image.getpixel((x_right + 1, y)) == target_color:
            x_right += 1

        for xi in range(x_left, x_right + 1):
            self.image.putpixel((xi, y), color) 

        for xi in range(x_left, x_right + 1):
            self._flood_fill_recursive(xi, y - 1, color, target_color)

        for xi in range(x_left, x_right + 1):
            self._flood_fill_recursive(xi, y + 1, color, target_color)
        

    def fill_pattern_dialog(self):
        filename = filedialog.askopenfilename(
            title="Выберите изображение для заливки",
            filetypes=[("Image files", "*.png *.jpg *.jpeg *.bmp *.gif")]
        )
        if filename:
            try:
                self.pattern_image = Image.open(filename)
                self.status_label.config(text=f"Загружено: {filename.split('/')[-1]}")
                # Запуск заливки после выбора точки и загрузки рисунка
                if hasattr(self, 'fill_point'):
                    x, y = self.fill_point
                    target_color = self.image.getpixel((x, y))
                    self._flood_fill_pattern_recursive(x, y, target_color, self.pattern_image)
                    self.photo = ImageTk.PhotoImage(self.image)
                    self.canvas.itemconfig(self.canvas_image, image=self.photo)
            except Exception as e:
                messagebox.showerror("Ошибка", f"Не удалось загрузить изображение: {e}")

    # --- Рекурсивная заливка линиями с шаблоном ---
    def _flood_fill_pattern_recursive(self, x, y, target_color, pattern):
        pattern_width, pattern_height = pattern.size
        if x < 0 or x >= self.canvas_width or y < 0 or y >= self.canvas_height:
            return
        current_color = self.image.getpixel((x, y))
        if current_color != target_color:
            return

        x_left = x
        while x_left > 0 and self.image.getpixel((x_left - 1, y)) == target_color:
            x_left -= 1

        x_right = x
        while x_right < self.canvas_width - 1 and self.image.getpixel((x_right + 1, y)) == target_color:
            x_right += 1

        for xi in range(x_left, x_right + 1):
            px = xi % pattern_width
            py = y % pattern_height
            self.draw.point((xi, y), fill=pattern.getpixel((px, py)))

        for xi in range(x_left, x_right + 1):
            self._flood_fill_pattern_recursive(xi, y - 1, target_color, pattern)

        for xi in range(x_left, x_right + 1):
            self._flood_fill_pattern_recursive(xi, y + 1, target_color, pattern)

    def detect_border(self):
        if hasattr(self, 'fill_point'):
            messagebox.showinfo("Инфо", "Алгоритм обнаружения границы будет реализован здесь")
        else:
            messagebox.showwarning("Внимание", "Сначала установите точку заливки (правый клик)")
    
    def show_border(self):
        messagebox.showinfo("Инфо", "Отображение границы будет реализовано здесь")
    
    def clear_canvas(self):
        self.image = Image.new("RGB", (self.canvas_width, self.canvas_height), "white")
        self.draw = ImageDraw.Draw(self.image)
        self.photo = ImageTk.PhotoImage(self.image)
        self.canvas.itemconfig(self.canvas_image, image=self.photo)
        self.canvas.delete("fill_point")
        self.status_label.config(text="Холст очищен")
    
    def load_image(self):
        filename = filedialog.askopenfilename(
            title="Выберите изображение",
            filetypes=[("Image files", "*.png *.jpg *.jpeg *.bmp *.gif")]
        )
        if filename:
            try:
                loaded_image = Image.open(filename)
                if loaded_image.size != (self.canvas_width, self.canvas_height):
                    loaded_image = loaded_image.resize((self.canvas_width, self.canvas_height))
                
                self.image = loaded_image.copy()
                self.draw = ImageDraw.Draw(self.image)
                self.photo = ImageTk.PhotoImage(self.image)
                self.canvas.itemconfig(self.canvas_image, image=self.photo)
                self.status_label.config(text="Изображение загружено")
            except Exception as e:
                messagebox.showerror("Ошибка", f"Не удалось загрузить изображение: {e}")
        