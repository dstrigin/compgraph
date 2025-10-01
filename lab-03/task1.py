import tkinter as tk
from tkinter import ttk, filedialog, messagebox, colorchooser
from PIL import Image, ImageTk, ImageDraw
import numpy as np
from collections import deque

class ColorFiller:
    def __init__(self, root):
        self.root = root
        self.root.title("Преобразование в оттенки серого")
        self.root.geometry("1000x750+200+100")
        self.root.minsize(800, 600)
        
        self.drawing = False
        self.last_x, self.last_y = None, None
        self.fill_color = "blue"
        self.border_color = (255, 0, 0)
        self.pattern_image = None
        self.border_points = []
        
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
        
        ttk.Button(border_frame, text="Выбрать цвет границы", 
                  command=self.choose_border_color).pack(fill=tk.X, pady=5)
        
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
        self.canvas.delete("fill_point")
        
        self.fill_point = (event.x, event.y)
        self.canvas.create_oval(event.x-3, event.y-3, event.x+3, event.y+3, 
                               fill="red", outline="red", tags="fill_point")
        self.status_label.config(text=f"Точка заливки: ({event.x}, {event.y})")
    
    def choose_border_color(self):
        """Выбор цвета для выделения границы"""
        color_tuple = colorchooser.askcolor(title="Выберите цвет для выделения границы")
        if color_tuple and color_tuple[1]:
            hex_color = color_tuple[1].lstrip('#')
            self.border_color = tuple(int(hex_color[i:i+2], 16) for i in (0, 2, 4))
            self.status_label.config(text=f"Цвет границы установлен")
    
    def detect_border(self):
        """Выделение границы связной области с исправлением ошибки цвета"""
        if not hasattr(self, 'fill_point'):
            messagebox.showwarning("Внимание", "Сначала установите точку на границе (правый клик)")
            return
        
        x, y = self.fill_point
        
        if x < 0 or x >= self.canvas_width or y < 0 or y >= self.canvas_height:
            messagebox.showerror("Ошибка", "Точка заливки находится вне холста")
            return
        
        try:
            border_pixel_color = self.image.getpixel((x, y))
            
            self.border_points = self._trace_boundary(x, y, border_pixel_color)
            
            if not self.border_points:
                messagebox.showinfo("Инфо", "Граница не найдена")
                return
            
            for point in self.border_points:
                self.image.putpixel(point, self.border_color)
            
            self.photo = ImageTk.PhotoImage(self.image)
            self.canvas.itemconfig(self.canvas_image, image=self.photo)
            
            self.status_label.config(text=f"Граница выделена: {len(self.border_points)} точек")
            
        except Exception as e:
            messagebox.showerror("Ошибка", f"Ошибка при выделении границы: {str(e)}")
    
    def _trace_boundary(self, start_x, start_y, boundary_color):
        """Алгоритм трассировки границы с использованием 8-связной области"""
        directions = [(-1, -1), (0, -1), (1, -1), 
                     (-1, 0),          (1, 0),
                     (-1, 1),  (0, 1),  (1, 1)]
        
        if self.image.getpixel((start_x, start_y)) != boundary_color:
            for dx, dy in directions:
                nx, ny = start_x + dx, start_y + dy
                if (0 <= nx < self.canvas_width and 0 <= ny < self.canvas_height and 
                    self.image.getpixel((nx, ny)) == boundary_color):
                    start_x, start_y = nx, ny
                    break
            else:
                return []
        
        boundary_points = []
        visited = set()
        
        queue = deque([(start_x, start_y)])
        visited.add((start_x, start_y))
        
        while queue:
            x, y = queue.popleft()
            boundary_points.append((x, y))
            
            for dx, dy in directions:
                nx, ny = x + dx, y + dy
                
                if (0 <= nx < self.canvas_width and 0 <= ny < self.canvas_height and 
                    (nx, ny) not in visited and 
                    self.image.getpixel((nx, ny)) == boundary_color):
                    
                    visited.add((nx, ny))
                    queue.append((nx, ny))
        
        return boundary_points
    
    def show_border(self):
        """Показать границу поверх изображения"""
        if not self.border_points:
            messagebox.showinfo("Инфо", "Сначала выделите границу")
            return
        
        temp_image = self.image.copy()
        
        for point in self.border_points:
            temp_image.putpixel(point, self.border_color)
        
        temp_photo = ImageTk.PhotoImage(temp_image)
        self.canvas.itemconfig(self.canvas_image, image=temp_photo)
        self.canvas.image = temp_photo
        
        self.status_label.config(text="Граница показана поверх изображения")
    
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
            pattern_pixel = pattern.getpixel((px, py))
            if isinstance(pattern_pixel, int):
                pattern_pixel = (pattern_pixel, pattern_pixel, pattern_pixel)
            elif len(pattern_pixel) == 4:
                pattern_pixel = pattern_pixel[:3]
            self.image.putpixel((xi, y), pattern_pixel)

        for xi in range(x_left, x_right + 1):
            self._flood_fill_pattern_recursive(xi, y - 1, target_color, pattern)

        for xi in range(x_left, x_right + 1):
            self._flood_fill_pattern_recursive(xi, y + 1, target_color, pattern)

    def clear_canvas(self):
        self.image = Image.new("RGB", (self.canvas_width, self.canvas_height), "white")
        self.draw = ImageDraw.Draw(self.image)
        self.photo = ImageTk.PhotoImage(self.image)
        self.canvas.itemconfig(self.canvas_image, image=self.photo)
        self.canvas.delete("fill_point")
        self.border_points = []
        self.status_label.config(text="Холст очищен")
    
    def load_image(self):
        filename = filedialog.askopenfilename(
            title="Выберите изображение",
            filetypes=[("Image files", "*.png *.jpg *.jpeg *.bmp *.gif")]
        )
        if filename:
            try:
                loaded_image = Image.open(filename)
                if loaded_image.mode != 'RGB':
                    loaded_image = loaded_image.convert('RGB')
                
                if loaded_image.size != (self.canvas_width, self.canvas_height):
                    loaded_image = loaded_image.resize((self.canvas_width, self.canvas_height))
                
                self.image = loaded_image.copy()
                self.draw = ImageDraw.Draw(self.image)
                self.photo = ImageTk.PhotoImage(self.image)
                self.canvas.itemconfig(self.canvas_image, image=self.photo)
                self.border_points = []
                self.status_label.config(text="Изображение загружено")
            except Exception as e:
                messagebox.showerror("Ошибка", f"Не удалось загрузить изображение: {e}")

if __name__ == "__main__":
    root = tk.Tk()
    app = ColorFiller(root)
    root.mainloop()